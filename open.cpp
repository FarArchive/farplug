#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

class FileStream: public IInStream, public UnknownImpl {
private:
  HANDLE h_file;
  wstring file_path;
  Error& error;
public:
  FileStream(const wstring& file_path, Error& error): file_path(file_path), error(error) {
    h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_POSIX_SEMANTICS, NULL);
    CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
  }
  ~FileStream() {
    CloseHandle(h_file);
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialInStream)
  UNKNOWN_IMPL_ITF(IInStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    ERROR_MESSAGE_BEGIN
    DWORD bytes_read;
    CHECK_SYS(ReadFile(h_file, data, size, &bytes_read, NULL));
    if (processedSize)
      *processedSize = bytes_read;
    return S_OK;
    ERROR_MESSAGE_END(file_path)
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
    COM_ERROR_HANDLER_BEGIN
    ERROR_MESSAGE_BEGIN
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
    ERROR_MESSAGE_END(file_path)
    COM_ERROR_HANDLER_END
  }
};


class ArchiveOpener: public IArchiveOpenCallback, public IArchiveOpenVolumeCallback, public ICryptoGetTextPassword, public UnknownImpl, public ProgressMonitor {
private:
  Archive& archive;
  UInt64 total_files;
  UInt64 total_bytes;
  UInt64 completed_files;
  UInt64 completed_bytes;
  FindData volume_file_info;

  virtual void do_update_ui() {
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
public:
  Error error;

  ArchiveOpener(Archive& archive): archive(archive), volume_file_info(archive.archive_file_info), total_files(0), total_bytes(0), completed_files(0), completed_bytes(0) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IArchiveOpenCallback)
  UNKNOWN_IMPL_ITF(IArchiveOpenVolumeCallback)
  UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
  UNKNOWN_IMPL_END

  STDMETHODIMP SetTotal(const UInt64 *files, const UInt64 *bytes) {
    COM_ERROR_HANDLER_BEGIN
    if (files) total_files = *files;
    if (bytes) total_bytes = *bytes;
    update_ui();
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetCompleted(const UInt64 *files, const UInt64 *bytes) {
    COM_ERROR_HANDLER_BEGIN
    if (files) completed_files = *files;
    if (bytes) completed_bytes = *bytes;
    update_ui();
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP GetProperty(PROPID propID, PROPVARIANT *value) {
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

  STDMETHODIMP GetStream(const wchar_t *name, IInStream **inStream) {
    COM_ERROR_HANDLER_BEGIN
    wstring file_path = add_trailing_slash(archive.archive_dir) + name;
    ERROR_MESSAGE_BEGIN
    try {
      volume_file_info = get_find_data(file_path);
    }
    catch (Error&) {
      return S_FALSE;
    }
    if (volume_file_info.is_dir())
      return S_FALSE;
    ComObject<IInStream> file_stream(new FileStream(file_path, error));
    file_stream.detach(inStream);
    update_ui();
    return S_OK;
    ERROR_MESSAGE_END(file_path)
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP CryptoGetTextPassword(BSTR *password) {
    COM_ERROR_HANDLER_BEGIN
    if (archive.password.empty()) {
      ProgressSuspend ps(*this);
      if (!password_dialog(archive.password))
        FAIL(E_ABORT);
    }
    *password = str_to_bstr(archive.password);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


void detect(const ArcFormats& arc_formats, IInStream* in_stream, ArchiveOpener* opener, vector<ComObject<IInArchive>>& archives, vector<wstring>& format_names) {
  wstring parent_prefix;
  if (!format_names.empty()) parent_prefix = format_names.back() + L" -> ";
  for (ArcFormats::const_iterator arc_format = arc_formats.begin(); arc_format != arc_formats.end(); arc_format++) {
    ComObject<IInArchive> in_arc;
    CHECK_COM(arc_format->arc_lib->CreateObject(reinterpret_cast<const GUID*>(arc_format->class_id.data()), &IID_IInArchive, reinterpret_cast<void**>(&in_arc)));

    const UInt64 max_check_start_position = 1 << 20;
    CHECK_COM(in_stream->Seek(0, STREAM_SEEK_SET, NULL));

    HRESULT res = in_arc->Open(in_stream, &max_check_start_position, opener);
    if (FAILED(res)) {
      if (opener->error.code != NO_ERROR)
        throw opener->error;
      CHECK_COM(res);
    }

    if (res == S_OK) {
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

      detect(arc_formats, sub_stream, opener, archives, format_names);
    }
  }
}

bool Archive::open() {
  ComObject<ArchiveOpener> opener(new ArchiveOpener(*this));
  ComObject<FileStream> file_stream(new FileStream(add_trailing_slash(archive_dir) + archive_file_info.cFileName, opener->error));
  vector<ComObject<IInArchive>> archives;
  vector<wstring> format_names;
  detect(arc_formats, file_stream, opener, archives, format_names);

  if (format_names.size() == 0) return false;

  int format_idx;
  if (format_names.size() == 1)
    format_idx = 0;
  else
    format_idx = Far::menu(L"", format_names);
  if (format_idx == -1) return false;

  in_arc = archives[format_idx];
  return true;
}
