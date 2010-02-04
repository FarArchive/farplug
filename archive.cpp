#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "ui.hpp"
#include "archive.hpp"

HRESULT ArcLib::get_bool_prop(UInt32 index, PROPID prop_id, bool& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, &prop);
  if (FAILED(res))
    return res;
  if (prop.vt == VT_BOOL)
    value = prop.boolVal == VARIANT_TRUE;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT ArcLib::get_string_prop(UInt32 index, PROPID prop_id, wstring& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, &prop);
  if (FAILED(res))
    return res;
  if (prop.vt == VT_BSTR)
    value = prop.bstrVal;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT ArcLib::get_bytes_prop(UInt32 index, PROPID prop_id, string& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, &prop);
  if (FAILED(res))
    return res;
  if (prop.vt == VT_BSTR) {
    UINT len = SysStringByteLen(prop.bstrVal);
    value.assign(reinterpret_cast<string::const_pointer>(prop.bstrVal), len);
  }
  else
    return E_FAIL;
  return S_OK;
}

void ArcLibs::load(const wstring& path) {
  FileEnum file_enum(path);
  while (file_enum.next()) {
    ArcLib arc_lib;
    arc_lib.h_module = LoadLibraryW((add_trailing_slash(path) + file_enum.data().cFileName).c_str());
    if (arc_lib.h_module) {
      arc_lib.CreateObject = reinterpret_cast<ArcLib::FCreateObject>(GetProcAddress(arc_lib.h_module, "CreateObject"));
      arc_lib.GetNumberOfMethods = reinterpret_cast<ArcLib::FGetNumberOfMethods>(GetProcAddress(arc_lib.h_module, "GetNumberOfMethods"));
      arc_lib.GetMethodProperty = reinterpret_cast<ArcLib::FGetMethodProperty>(GetProcAddress(arc_lib.h_module, "GetMethodProperty"));
      arc_lib.GetNumberOfFormats = reinterpret_cast<ArcLib::FGetNumberOfFormats>(GetProcAddress(arc_lib.h_module, "GetNumberOfFormats"));
      arc_lib.GetHandlerProperty = reinterpret_cast<ArcLib::FGetHandlerProperty>(GetProcAddress(arc_lib.h_module, "GetHandlerProperty"));
      arc_lib.GetHandlerProperty2 = reinterpret_cast<ArcLib::FGetHandlerProperty2>(GetProcAddress(arc_lib.h_module, "GetHandlerProperty2"));
      if (arc_lib.CreateObject && arc_lib.GetNumberOfFormats && arc_lib.GetHandlerProperty2) {
        push_back(arc_lib);
      }
      else {
        FreeLibrary(arc_lib.h_module);
      }
    }
  }
}

ArcLibs::~ArcLibs() {
  for (const_iterator arc_lib = begin(); arc_lib != end(); arc_lib++) {
    FreeLibrary(arc_lib->h_module);
  }
  clear();
}

void ArcFormats::load(const ArcLibs& arc_libs) {
  for (ArcLibs::const_iterator arc_lib = arc_libs.begin(); arc_lib != arc_libs.end(); arc_lib++) {
    UInt32 num_formats;
    if (arc_lib->GetNumberOfFormats(&num_formats) == S_OK) {
      for (UInt32 idx = 0; idx < num_formats; idx++) {
        ArcFormat arc_format;
        arc_format.arc_lib = &*arc_lib;
        CHECK_COM(arc_lib->get_string_prop(idx, NArchive::kName, arc_format.name));
        CHECK_COM(arc_lib->get_bytes_prop(idx, NArchive::kClassID, arc_format.class_id));
        CHECK_COM(arc_lib->get_bool_prop(idx, NArchive::kUpdate, arc_format.update));
        arc_lib->get_bytes_prop(idx, NArchive::kStartSignature, arc_format.start_signature);
        arc_lib->get_string_prop(idx, NArchive::kExtension, arc_format.extension);
        push_back(arc_format);
      }
    }
  }
}

class FileStream: public IInStream, public UnknownImpl {
private:
  HANDLE h_file;
public:
  FileStream(const wstring& file_path);
  ~FileStream();

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IInStream)
  UNKNOWN_IMPL_ITF(ISequentialInStream)
  UNKNOWN_IMPL_END

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
};

FileStream::FileStream(const wstring& file_path) {
  h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_POSIX_SEMANTICS, NULL);
  CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
}

FileStream::~FileStream() {
  CloseHandle(h_file);
}

STDMETHODIMP FileStream::Read(void *data, UInt32 size, UInt32 *processedSize) {
  COM_ERROR_HANDLER_BEGIN
  DWORD bytes_read;
  CHECK_SYS(ReadFile(h_file, data, size, &bytes_read, NULL));
  if (processedSize)
    *processedSize = bytes_read;
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP FileStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
  COM_ERROR_HANDLER_BEGIN
  DWORD move_method;
  switch (seekOrigin) {
  case STREAM_SEEK_SET:
    move_method = FILE_BEGIN;
    break;
  case STREAM_SEEK_CUR:
    move_method = FILE_CURRENT;
    break;
  case STREAM_SEEK_END:
    move_method = FILE_END;
    break;
  default:
    return E_INVALIDARG;
  }
  LARGE_INTEGER distance;
  distance.QuadPart = offset;
  LARGE_INTEGER new_position;
  CHECK_SYS(SetFilePointerEx(h_file, distance, &new_position, move_method));
  if (newPosition)
    *newPosition = new_position.QuadPart;
  return S_OK;
  COM_ERROR_HANDLER_END
}

class FileOutStream: public IOutStream, public UnknownImpl {
private:
  HANDLE h_file;
  FileInfo file_info;
public:
  FileOutStream(const wstring& file_path, const FileInfo& file_info);
  ~FileOutStream();

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_ITF(IOutStream)
  UNKNOWN_IMPL_END

  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(SetSize)(Int64 newSize);
};

FileOutStream::FileOutStream(const wstring& file_path, const FileInfo& file_info): file_info(file_info) {
  h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, file_info.attr | FILE_FLAG_POSIX_SEMANTICS, NULL);
  CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
}

FileOutStream::~FileOutStream() {
  SetFileTime(h_file, &file_info.ctime, &file_info.atime, &file_info.mtime);
  CloseHandle(h_file);
}

STDMETHODIMP FileOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
  COM_ERROR_HANDLER_BEGIN
  DWORD move_method;
  switch (seekOrigin) {
  case STREAM_SEEK_SET:
    move_method = FILE_BEGIN; break;
  case STREAM_SEEK_CUR:
    move_method = FILE_CURRENT; break;
  case STREAM_SEEK_END:
    move_method = FILE_END; break;
  default:
    FAIL(E_INVALIDARG);
  }
  LARGE_INTEGER distance;
  distance.QuadPart = offset;
  LARGE_INTEGER new_position;
  CHECK_SYS(SetFilePointerEx(h_file, distance, &new_position, move_method));
  if (newPosition)
    *newPosition = new_position.QuadPart;
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP FileOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize) {
  COM_ERROR_HANDLER_BEGIN
  CHECK_SYS(WriteFile(h_file, data, size, reinterpret_cast<LPDWORD>(processedSize), NULL));
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP FileOutStream::SetSize(Int64 newSize) {
  COM_ERROR_HANDLER_BEGIN
  LARGE_INTEGER position;
  position.QuadPart = newSize;
  CHECK_SYS(SetFilePointerEx(h_file, position, NULL, FILE_BEGIN));
  CHECK_SYS(SetEndOfFile(h_file));
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveOpenCallback::SetTotal(const UInt64 *files, const UInt64 *bytes) {
  COM_ERROR_HANDLER_BEGIN
  if (files) total_files = *files;
  if (bytes) total_bytes = *bytes;
  update_ui();
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveOpenCallback::SetCompleted(const UInt64 *files, const UInt64 *bytes) {
  COM_ERROR_HANDLER_BEGIN
  if (files) completed_files = *files;
  if (bytes) completed_bytes = *bytes;
  update_ui();
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveOpenCallback::GetProperty(PROPID propID, PROPVARIANT *value) {
  COM_ERROR_HANDLER_BEGIN
  PropVariant var;
  switch (propID) {
  case kpidName:
    var = volume_file_info.cFileName; break;
  case kpidIsDir:
    var = volume_file_info.is_dir(); break;
  case kpidSize:
    var = volume_file_info.size(); break;
  case kpidAttrib:
    var = static_cast<UInt32>(volume_file_info.dwFileAttributes); break;
  case kpidCTime:
    var = volume_file_info.ftCreationTime; break;
  case kpidATime:
    var = volume_file_info.ftLastAccessTime; break;
  case kpidMTime:
    var = volume_file_info.ftLastWriteTime; break;
  }
  var.detach(value);
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveOpenCallback::GetStream(const wchar_t *name, IInStream **inStream) {
  COM_ERROR_HANDLER_BEGIN
  wstring file_path = add_trailing_slash(reader.archive_dir) + name;
  try {
    volume_file_info = get_find_data(file_path);
  }
  catch (Error&) {
    return S_FALSE;
  }
  if (volume_file_info.is_dir())
    return S_FALSE;
  ComObject<IInStream> file_stream(new FileStream(file_path));
  file_stream.detach(inStream);
  update_ui();
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveOpenCallback::CryptoGetTextPassword(BSTR *password) {
  COM_ERROR_HANDLER_BEGIN
  if (reader.password.empty()) {
    ProgressSuspend ps(*this);
    if (!password_dialog(reader.password))
      FAIL(E_ABORT);
  }
  *password = str_to_bstr(reader.password);
  return S_OK;
  COM_ERROR_HANDLER_END
}

void ArchiveOpenCallback::do_update_ui() {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << volume_file_info.cFileName << L'\n';
  st << completed_files << L" / " << total_files << L'\n';
  st << Far::get_progress_bar_str(60, completed_files, total_files) << L'\n';
  st << L"\x01\n";
  st << format_data_size(completed_bytes, get_size_suffixes()) << L" / " << format_data_size(total_bytes, get_size_suffixes()) << L'\n';
  st << Far::get_progress_bar_str(60, completed_bytes, total_bytes) << L'\n';
  Far::message(st.str(), 0, FMSG_LEFTALIGN);
}

STDMETHODIMP ArchiveExtractCallback::SetTotal(UInt64 total) {
  COM_ERROR_HANDLER_BEGIN
  this->total = total;
  update_ui();
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveExtractCallback::SetCompleted(const UInt64 *completeValue) {
  COM_ERROR_HANDLER_BEGIN
  if (completeValue) this->completed = *completeValue;
  update_ui();
  return S_OK;
  COM_ERROR_HANDLER_END
}

FindData convert_file_info(const FileInfo& file_info) {
  FindData find_data;
  memset(&find_data, 0, sizeof(find_data));
  find_data.dwFileAttributes = file_info.attr;
  find_data.ftCreationTime = file_info.ctime;
  find_data.ftLastAccessTime = file_info.atime;
  find_data.ftLastWriteTime = file_info.mtime;
  find_data.nFileSizeHigh = file_info.size >> 32;
  find_data.nFileSizeLow = file_info.size & 0xFFFFFFFF;
  wcscpy(find_data.cFileName, file_info.name.c_str());
  return find_data;
}

STDMETHODIMP ArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream,  Int32 askExtractMode) {
  COM_ERROR_HANDLER_BEGIN
  DEBUG_OUTPUT(L"ArchiveExtractCallback::GetStream(" + int_to_str(index) + L", " + int_to_str(askExtractMode) + L")");

  const FileInfo& file_info = reader.file_list[reader.file_id_index[index]];
  file_path = file_info.name;
  UInt32 index = file_info.parent;
  while (index != src_dir_index) {
    const FileInfo& file_info = reader.file_list[reader.file_id_index[index]];
    file_path.insert(0, 1, L'\\').insert(0, file_info.name);
    index = file_info.parent;
  }
  file_path.insert(0, 1, L'\\').insert(0, dest_dir);

  FindData dst_file_info;
  HANDLE h_find = FindFirstFileW(long_path(file_path).c_str(), &dst_file_info);
  if (h_find != INVALID_HANDLE_VALUE) {
    FindClose(h_find);
    bool overwrite;
    if (oo == ooAsk) {
      FindData src_file_info = convert_file_info(file_info);
      OverwriteAction oa = overwrite_dialog(file_path, src_file_info, dst_file_info);
      if (oa == oaYes)
        overwrite = true;
      else if (oa == oaYesAll) {
        overwrite = true;
        oo = ooOverwrite;
      }
      else if (oa == oaNo)
        overwrite = false;
      else if (oa == oaNoAll) {
        overwrite = false;
        oo = ooSkip;
      }
      else if (oa == oaCancel)
        return E_ABORT;
    }
    else if (oo == ooOverwrite)
      overwrite = true;
    else if (oo == ooSkip)
      overwrite = false;

    if (overwrite) {
      CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), FILE_ATTRIBUTE_NORMAL));
    }
    else {
      *outStream = NULL;
      return S_OK;
    }
  }

  ComObject<ISequentialOutStream> file_out_stream(new FileOutStream(file_path, file_info));
  file_out_stream.detach(outStream);
  update_ui();
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveExtractCallback::PrepareOperation(Int32 askExtractMode) {
  COM_ERROR_HANDLER_BEGIN
  DEBUG_OUTPUT(L"ArchiveExtractCallback::PrepareOperation(" + int_to_str(askExtractMode) + L")");
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveExtractCallback::SetOperationResult(Int32 resultEOperationResult) {
  COM_ERROR_HANDLER_BEGIN
  DEBUG_OUTPUT(L"ArchiveExtractCallback::SetOperationResult(" + int_to_str(resultEOperationResult) + L")");
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveExtractCallback::CryptoGetTextPassword(BSTR *password) {
  COM_ERROR_HANDLER_BEGIN
  if (reader.password.empty()) {
    ProgressSuspend ps(*this);
    if (!password_dialog(reader.password))
      FAIL(E_ABORT);
  }
  *password = str_to_bstr(reader.password);
  return S_OK;
  COM_ERROR_HANDLER_END
}

void ArchiveExtractCallback::do_update_ui() {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << fit_str(file_path, 60) << L'\n';
  st << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L'\n';
  st << Far::get_progress_bar_str(60, completed, total) << L'\n';
  Far::message(st.str(), 0, FMSG_LEFTALIGN);
}

void ArchiveReader::detect(IInStream* in_stream, IArchiveOpenCallback* callback, vector<ComObject<IInArchive>>& archives, vector<wstring>& format_names) {
  wstring parent_prefix;
  if (!format_names.empty()) parent_prefix = format_names.back() + L" -> ";
  for (ArcFormats::const_iterator arc_format = arc_formats.begin(); arc_format != arc_formats.end(); arc_format++) {
    ComObject<IInArchive> in_arc;
    CHECK_COM(arc_format->arc_lib->CreateObject(reinterpret_cast<const GUID*>(arc_format->class_id.data()), &IID_IInArchive, reinterpret_cast<void**>(&in_arc)));

    const UInt64 max_check_start_position = 1 << 20;
    CHECK_COM(in_stream->Seek(0, STREAM_SEEK_SET, NULL));
    if (s_ok(in_arc->Open(in_stream, &max_check_start_position, callback))) {
      archives.push_back(in_arc);
      format_names.push_back(parent_prefix + arc_format->name);

      UInt32 main_subfile;
      PropVariant var;
      if (in_arc->GetArchiveProperty(kpidMainSubfile, &var) != S_OK || var.vt != VT_UI4)
        continue;
      main_subfile = var.ulVal;
      UInt32 num_items;
      if (in_arc->GetNumberOfItems(&num_items) != S_OK || main_subfile >= num_items)
        continue;

      ComObject<IInArchiveGetStream> get_stream;
      if (in_arc->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void**>(&get_stream)) != S_OK || !get_stream)
        continue;

      ComObject<ISequentialInStream> sub_seq_stream;
      if (get_stream->GetStream(main_subfile, &sub_seq_stream) != S_OK || !sub_seq_stream)
        continue;

      ComObject<IInStream> sub_stream;
      if (sub_seq_stream->QueryInterface(IID_IInStream, reinterpret_cast<void**>(&sub_stream)) != S_OK || !sub_stream)
        continue;

      detect(sub_stream, callback, archives, format_names);
    }
  }
}

bool ArchiveReader::open() {
  ComObject<FileStream> file_stream(new FileStream(add_trailing_slash(archive_dir) + archive_file_info.cFileName));
  ComObject<ArchiveOpenCallback> archive_open_callback(new ArchiveOpenCallback(*this));
  vector<ComObject<IInArchive>> archives;
  vector<wstring> format_names;
  detect(file_stream, archive_open_callback, archives, format_names);

  if (format_names.size() == 0) return false;

  int format_idx;
  if (format_names.size() == 1)
    format_idx = 0;
  else
    format_idx = Far::menu(L"", format_names);
  if (format_idx == -1) return false;

  archive = archives[format_idx];
  return true;
}

ArchiveReader::ArchiveReader(const ArcFormats& arc_formats, const wstring& file_path): arc_formats(arc_formats), archive_file_info(get_find_data(file_path)), archive_dir(extract_file_path(file_path)) {
}

wstring ArchiveReader::get_default_name() const {
  wstring name = archive_file_info.cFileName;
  size_t pos = name.find_last_of(L'.');
  if (pos == wstring::npos)
    return name;
  else
    return name.substr(0, pos);
}

void ArchiveReader::make_index() {
  class Progress: public ProgressMonitor {
  private:
    UInt32 completed;
    UInt32 total;
    virtual void do_update_ui() {
      wostringstream st;
      st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
      st << completed << L" / " << total << L'\n';
      st << Far::get_progress_bar_str(60, completed, total) << L'\n';
      Far::message(st.str(), 0, FMSG_LEFTALIGN);
    }
  public:
    Progress(): completed(0), total(0) {
    }
    void update(UInt32 completed, UInt32 total) {
      this->completed = completed;
      this->total = total;
      update_ui();
    }
  };
  Progress progress;

  UInt32 num_items = 0;
  CHECK_COM(archive->GetNumberOfItems(&num_items));
  file_list.clear();
  file_list.reserve(num_items);

  struct DirCmp: public binary_function<FileInfo, FileInfo, bool> {
    bool operator()(const FileInfo& left, const FileInfo& right) const {
      if (left.parent == right.parent)
        return left.name < right.name;
      else
        return left.parent < right.parent;
    }
  };
  typedef set<FileInfo, DirCmp> DirList;
  DirList dir_list;

  FileInfo dir_info;
  UInt32 fake_dir_index = num_items;
  FileInfo file_info;
  wstring path;
  PropVariant var;
  for (UInt32 i = 0; i < num_items; i++) {
    progress.update(i, num_items);

    if (s_ok(archive->GetProperty(i, kpidPath, &var)) && var.vt == VT_BSTR)
      path.assign(var.bstrVal);
    else
      path.assign(get_default_name());

    file_info.index = i;

    // attributes
    bool is_dir = s_ok(archive->GetProperty(i, kpidIsDir, &var)) && var.vt == VT_BOOL && var.boolVal;
    if (s_ok(archive->GetProperty(i, kpidAttrib, &var)) && var.vt == VT_UI4)
      file_info.attr = var.ulVal;
    else
      file_info.attr = 0;
    if (is_dir)
      file_info.attr |= FILE_ATTRIBUTE_DIRECTORY;
    else
      is_dir = file_info.is_dir();

    // size
    if (!is_dir && s_ok(archive->GetProperty(i, kpidSize, &var)) && var.vt == VT_UI8)
      file_info.size = var.uhVal.QuadPart;
    else
      file_info.size = 0;
    if (!is_dir && s_ok(archive->GetProperty(i, kpidPackSize, &var)) && var.vt == VT_UI8)
      file_info.psize = var.uhVal.QuadPart;
    else
      file_info.psize = 0;

    // date & time
    if (s_ok(archive->GetProperty(i, kpidCTime, &var)) && var.vt == VT_FILETIME)
      file_info.ctime = var.filetime;
    else
      file_info.ctime = archive_file_info.ftCreationTime;
    if (s_ok(archive->GetProperty(i, kpidMTime, &var)) && var.vt == VT_FILETIME)
      file_info.mtime = var.filetime;
    else
      file_info.mtime = archive_file_info.ftLastWriteTime;
    if (s_ok(archive->GetProperty(i, kpidATime, &var)) && var.vt == VT_FILETIME)
      file_info.atime = var.filetime;
    else
      file_info.atime = archive_file_info.ftLastAccessTime;

    // file name
    size_t name_end_pos = path.size();
    while (name_end_pos && is_slash(path[name_end_pos - 1])) name_end_pos--;
    size_t name_pos = name_end_pos;
    while (name_pos && !is_slash(path[name_pos - 1])) name_pos--;
    file_info.name.assign(path.data() + name_pos, name_end_pos - name_pos);

    // split path into individual directories and put them into DirList
    dir_info.parent = c_root_index;
    dir_info.attr = FILE_ATTRIBUTE_DIRECTORY;
    dir_info.size = dir_info.psize = 0;
    dir_info.ctime = archive_file_info.ftCreationTime;
    dir_info.mtime = archive_file_info.ftLastWriteTime;
    dir_info.atime = archive_file_info.ftLastAccessTime;
    size_t begin_pos = 0;
    while (begin_pos < name_pos) {
      dir_info.index = fake_dir_index;
      size_t end_pos = begin_pos;
      while (end_pos < name_pos && !is_slash(path[end_pos])) end_pos++;
      if (end_pos != begin_pos) {
        dir_info.name.assign(path.data() + begin_pos, end_pos - begin_pos);
        pair<DirList::iterator, bool> ins_pos = dir_list.insert(dir_info);
        if (ins_pos.second) {
          CHECK(fake_dir_index >= num_items && fake_dir_index != c_root_index);
          fake_dir_index++;
        }
        dir_info.parent = ins_pos.first->index;
      }
      begin_pos = end_pos + 1;
    }
    file_info.parent = dir_info.parent;

    if (is_dir) {
      pair<DirList::iterator, bool> ins_pos = dir_list.insert(file_info);
      if (!ins_pos.second) { // directory already present in DirList
        UInt32 old_index = ins_pos.first->index;
        *ins_pos.first = file_info;
        ins_pos.first->index = old_index;
      }
    }
    else {
      file_list.push_back(file_info);
    }
  }

  // add directories to file list
  file_list.reserve(file_list.size() + dir_list.size());
  copy(dir_list.begin(), dir_list.end(), back_insert_iterator<FileList>(file_list));
  struct DirListIndexCmp: public binary_function<FileInfo, FileInfo, bool> {
    bool operator()(const FileInfo& left, const FileInfo& right) const {
      if (left.parent == right.parent)
        return left.index < right.index;
      else
        return left.parent < right.parent;
    }
  };
  sort(file_list.begin(), file_list.end(), DirListIndexCmp());

  // map 7z file indices to internal file indices
  UInt32 max_index = 0;
  for (unsigned i = 0; i < file_list.size(); i++) {
    if (max_index < file_list[i].index)
      max_index = file_list[i].index;
  }
  file_id_index.clear();
  file_id_index.assign(max_index + 1, -1);
  for (unsigned i = 0; i < file_list.size(); i++) {
    file_id_index[file_list[i].index] = i;
  }

  // create directory search index
  dir_find_index.clear();
  dir_find_index.reserve(dir_list.size());
  for (size_t i = 0; i < file_list.size(); i++) {
    if (file_list[i].is_dir())
      dir_find_index.push_back(i);
  }
  struct DirFindIndexCmp: public binary_function<UInt32, UInt32, bool> {
  private:
    const FileList& file_list;
  public:
    DirFindIndexCmp(const FileList& file_list): file_list(file_list) {
    }
    bool operator()(const UInt32& left, const UInt32& right) const {
      if (file_list[left].parent == file_list[right].parent)
        return file_list[left].name < file_list[right].name;
      else
        return file_list[left].parent < file_list[right].parent;
    }
  };
  sort(dir_find_index.begin(), dir_find_index.end(), DirFindIndexCmp(file_list));
}

UInt32 ArchiveReader::dir_find(const wstring& path) {
  if (file_list.empty())
    make_index();

  struct DirFindIndexCmp: public binary_function<UInt32, UInt32, bool> {
  private:
    const FileList& file_list;
    FileInfo file_info;
  public:
    DirFindIndexCmp(const FileList& file_list, UInt32 parent_index, const wstring& name): file_list(file_list) {
      file_info.parent = parent_index;
      file_info.name = name;
    }
    bool operator()(const UInt32& left, const UInt32& right) const {
      const FileInfo& fi_left = left == -1 ? file_info : file_list[left];
      const FileInfo& fi_right = right == -1 ? file_info : file_list[right];
      if (fi_left.parent == fi_right.parent)
        return fi_left.name < fi_right.name;
      else
        return fi_left.parent < fi_right.parent;
    }
  };

  UInt32 parent_index = c_root_index;
  wstring name;
  size_t begin_pos = 0;
  while (begin_pos < path.size()) {
    size_t end_pos = begin_pos;
    while (end_pos < path.size() && !is_slash(path[end_pos])) end_pos++;
    if (end_pos != begin_pos) {
      name.assign(path.data() + begin_pos, end_pos - begin_pos);
      FileIndex::const_iterator dir_idx = lower_bound(dir_find_index.begin(), dir_find_index.end(), -1, DirFindIndexCmp(file_list, parent_index, name));
      CHECK(dir_idx != dir_find_index.end());
      const FileInfo& dir_info = file_list[*dir_idx];
      CHECK(dir_info.parent == parent_index && dir_info.name == name);
      parent_index = dir_info.index;
    }
    begin_pos = end_pos + 1;
  }
  return parent_index;
}

FileListRef ArchiveReader::dir_list(UInt32 dir_index) {
  if (file_list.empty())
    make_index();

  struct DirListIndexCmp: public binary_function<FileInfo, FileInfo, bool> {
    bool operator()(const FileInfo& left, const FileInfo& right) const {
      return left.parent < right.parent;
    }
  };
  FileInfo dir_info;
  dir_info.parent = dir_index;
  FileListRef fl_ref = equal_range(file_list.begin(), file_list.end(), dir_info, DirListIndexCmp());

  return fl_ref;
}

void ArchiveReader::prepare_extract(UInt32 dir_index, const wstring& dir_path, list<UInt32>& indices, ArchiveExtractCallback* progress) {
  BOOL res = CreateDirectoryW(long_path(dir_path).c_str(), NULL);
  if (!res) {
    CHECK_SYS(GetLastError() == ERROR_ALREADY_EXISTS);
  }

  FileListRef fl = dir_list(dir_index);
  for (FileList::const_iterator file_info = fl.first; file_info != fl.second; file_info++) {
    if (file_info->is_dir()) {
      prepare_extract(file_info->index, add_trailing_slash(dir_path) + file_info->name, indices, progress);
    }
    else
      indices.push_back(file_info->index);
  }
}

void ArchiveReader::set_dir_attr(FileInfo dir_info, const wstring& dir_path, ArchiveExtractCallback* progress) {
  FileListRef fl = dir_list(dir_info.index);
  for (FileList::const_iterator file_info = fl.first; file_info != fl.second; file_info++) {
    if (file_info->is_dir()) {
      set_dir_attr(*file_info, add_trailing_slash(dir_path) + file_info->name, progress);
    }
  }
  {
    CHECK_SYS(SetFileAttributesW(long_path(dir_path).c_str(), FILE_ATTRIBUTE_NORMAL));
    File open_dir(dir_path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS);
    CHECK_SYS(SetFileAttributesW(long_path(dir_path).c_str(), dir_info.attr));
    open_dir.set_time(&dir_info.ctime, &dir_info.atime, &dir_info.mtime);
  }
}

void ArchiveReader::extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const wstring& dest_dir) {
  ComObject<ArchiveExtractCallback> callback(new ArchiveExtractCallback(*this, src_dir_index, dest_dir));

  list<UInt32> file_indices;
  for (unsigned i = 0; i < src_indices.size(); i++) {
    const FileInfo& file_info = file_list[file_id_index[src_indices[i]]];
    if (file_info.is_dir()) {
      prepare_extract(file_info.index, add_trailing_slash(dest_dir) + file_info.name, file_indices, callback);
    }
    else {
      file_indices.push_back(file_info.index);
    }
  }

  vector<UInt32> indices;
  indices.reserve(file_indices.size());
  copy(file_indices.begin(), file_indices.end(), back_insert_iterator<vector<UInt32>>(indices));
  sort(indices.begin(), indices.end());
  CHECK_COM(archive->Extract(to_array(indices), indices.size(), 0, callback));

  for (unsigned i = 0; i < src_indices.size(); i++) {
    const FileInfo& file_info = file_list[file_id_index[src_indices[i]]];
    if (file_info.is_dir()) {
      set_dir_attr(file_info, add_trailing_slash(dest_dir) + file_info.name, callback);
    }
  }
}
