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

class ArchiveOpenCallback: public IArchiveOpenCallback, public UnknownImpl, public ProgressMonitor {
private:
  UInt64 total_files;
  UInt64 total_bytes;
  UInt64 completed_files;
  UInt64 completed_bytes;
  virtual void do_update_ui();
public:
  ArchiveOpenCallback(): total_files(0), total_bytes(0), completed_files(0), completed_bytes(0) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IArchiveOpenCallback)
  UNKNOWN_IMPL_END

  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);
};

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

void ArchiveOpenCallback::do_update_ui() {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << completed_files << L" / " << total_files << L'\n';
  st << Far::get_progress_bar_str(60, completed_files, total_files) << L'\n';
  st << L"\x01\n";
  st << format_data_size(completed_bytes, get_size_suffixes()) << L" / " << format_data_size(total_bytes, get_size_suffixes()) << L'\n';
  st << Far::get_progress_bar_str(60, completed_bytes, total_bytes) << L'\n';
  Far::message(st.str(), 0, FMSG_LEFTALIGN);
}

bool ArchiveReader::open(const ArcFormats& arc_formats, const wstring& file_path) {
  HANDLE h_find = FindFirstFileW(long_path(file_path).c_str(), &archive_file_info);
  CHECK_SYS(h_find != INVALID_HANDLE_VALUE);
  FindClose(h_find);

  ComObject<FileStream> file_stream(new FileStream(file_path));
  ComObject<ArchiveOpenCallback> archive_open_callback(new ArchiveOpenCallback());
  for (ArcFormats::const_iterator arc_format = arc_formats.begin(); arc_format != arc_formats.end(); arc_format++) {
    ComObject<IInArchive> in_arc;
    CHECK_COM(arc_format->arc_lib->CreateObject(reinterpret_cast<const GUID*>(arc_format->class_id.data()), &IID_IInArchive, reinterpret_cast<void**>(&in_arc)));

    const UInt64 max_check_start_position = 1 << 20;
    file_stream->Seek(0, STREAM_SEEK_SET, NULL);
    if (s_ok(in_arc->Open(file_stream, &max_check_start_position, archive_open_callback))) {
      archive = in_arc;
      return true;
    }
  }
  return false;
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
    if (is_dir) {
      file_info.size = file_info.psize = 0;
    }
    else {
      if (s_ok(archive->GetProperty(i, kpidSize, &var)) && var.vt == VT_UI8)
        file_info.size = var.uhVal.QuadPart;
      if (s_ok(archive->GetProperty(i, kpidPackSize, &var)) && var.vt == VT_UI8)
        file_info.psize = var.uhVal.QuadPart;
    }

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
  CHECK(fl_ref.first != file_list.end());

  return fl_ref;
}
