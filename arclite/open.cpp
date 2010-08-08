#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "msearch.hpp"
#include "archive.hpp"

class ArchiveOpenStream: public IInStream, public UnknownImpl {
private:
  HANDLE h_file;
  wstring file_path;
  Error& error;
public:
  ArchiveOpenStream(const wstring& file_path, Error& error): file_path(file_path), error(error) {
    h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
  }
  ~ArchiveOpenStream() {
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
  const wstring& archive_dir;
  FindData volume_file_info;
  wstring& password;
  Error& error;

  UInt64 total_files;
  UInt64 total_bytes;
  UInt64 completed_files;
  UInt64 completed_bytes;
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
  ArchiveOpener(const wstring& archive_dir, const FindData& archive_file_info, wstring& password, Error& error): archive_dir(archive_dir), volume_file_info(archive_file_info), password(password), error(error), total_files(0), total_bytes(0), completed_files(0), completed_bytes(0) {
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
    wstring file_path = add_trailing_slash(archive_dir) + name;
    ERROR_MESSAGE_BEGIN
    try {
      volume_file_info = get_find_data(file_path);
    }
    catch (Error&) {
      return S_FALSE;
    }
    if (volume_file_info.is_dir())
      return S_FALSE;
    ComObject<IInStream> file_stream(new ArchiveOpenStream(file_path, error));
    file_stream.detach(inStream);
    update_ui();
    return S_OK;
    ERROR_MESSAGE_END(file_path)
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP CryptoGetTextPassword(BSTR *pwd) {
    COM_ERROR_HANDLER_BEGIN
    if (password.empty()) {
      ProgressSuspend ps(*this);
      if (!password_dialog(password))
        FAIL(E_ABORT);
    }
    *pwd = str_to_bstr(password);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


bool Archive::open_sub_stream(IInArchive* in_arc, IInStream** sub_stream) {
  UInt32 main_subfile;
  PropVariant var;
  if (in_arc->GetArchiveProperty(kpidMainSubfile, &var) != S_OK || var.vt != VT_UI4)
    return false;
  main_subfile = var.ulVal;

  UInt32 num_items;
  if (in_arc->GetNumberOfItems(&num_items) != S_OK || main_subfile >= num_items)
    return false;

  ComObject<IInArchiveGetStream> get_stream;
  if (in_arc->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void**>(&get_stream)) != S_OK || !get_stream)
    return false;

  ComObject<ISequentialInStream> sub_seq_stream;
  if (get_stream->GetStream(main_subfile, &sub_seq_stream) != S_OK || !sub_seq_stream)
    return false;

  if (sub_seq_stream->QueryInterface(IID_IInStream, reinterpret_cast<void**>(sub_stream)) != S_OK || !sub_stream)
    return false;

  return true;
}

bool Archive::open_archive(IInStream* in_stream, IInArchive* archive) {
  CHECK_COM(in_stream->Seek(0, STREAM_SEEK_SET, NULL));

  Error error;
  ComObject<IArchiveOpenCallback> opener(new ArchiveOpener(archive_dir, archive_file_info, password, error));
  const UInt64 max_check_start_position = max_check_size;
  HRESULT res = archive->Open(in_stream, &max_check_start_position, opener);
  if (FAILED(res)) {
    if (error)
      throw error;
    else
      FAIL(res);
  }
  return res == S_OK;
}

void Archive::detect(IInStream* in_stream, bool all, vector<ArcFormatChain>& format_chains) {
  ArcFormatChain parent_format_chain;
  if (!format_chains.empty())
    parent_format_chain = format_chains.back();
  const ArcFormats& arc_formats = ArcAPI::get()->formats();

  vector<string> signatures;
  signatures.reserve(arc_formats.size());
  for_each(arc_formats.begin(), arc_formats.end(), [&] (const ArcFormat& arc_format) {
    signatures.push_back(arc_format.start_signature);
  });

  Buffer<unsigned char> buffer(max_check_size);
  UInt32 size;
  CHECK_COM(in_stream->Read(buffer.data(), buffer.size(), &size));

  vector<StrPos> sig_positions = msearch(buffer.data(), size, signatures);

  for (vector<StrPos>::const_iterator sig_pos = sig_positions.begin(); sig_pos != sig_positions.end(); sig_pos++) {
    const ArcFormat& arc_format = arc_formats[sig_pos->idx];
    ComObject<IInArchive> archive;
    ArcAPI::get()->create_in_archive(arc_format, &archive);
    if (open_archive(in_stream, archive)) {
      ArcFormatChain new_format_chain(parent_format_chain);
      new_format_chain.push_back(arc_format);
      format_chains.push_back(new_format_chain);

      ComObject<IInStream> sub_stream;
      if (open_sub_stream(archive, &sub_stream))
        detect(sub_stream, all, format_chains);

      if (!all) break;
    }
  };
}

vector<ArcFormatChain> Archive::detect(const wstring& file_path, bool all) {
  archive_file_info = get_find_data(file_path);
  archive_dir = extract_file_path(file_path);
  vector<ArcFormatChain> format_chains;
  Error error;
  ComObject<IInStream> stream(new ArchiveOpenStream(get_archive_path(), error));
  detect(stream, all, format_chains);
  return format_chains;
}

bool Archive::open(const wstring& file_path, const ArcFormatChain& format_chain) {
  Error error;
  ComObject<IInStream> stream(new ArchiveOpenStream(get_archive_path(), error));
  vector<ArcFormat>::const_iterator format = format_chain.begin();
  ArcAPI::get()->create_in_archive(*format, &in_arc);
  if (!open_archive(stream, in_arc))
    return false;
  format++;
  while (format != format_chain.end()) {
    ComObject<IInStream> sub_stream;
    CHECK(open_sub_stream(in_arc, &sub_stream));
    ArcAPI::get()->create_in_archive(*format, &in_arc);
    if (!open_archive(sub_stream, in_arc))
      return false;
    format++;
  }
  this->format_chain = format_chain;
  return true;
}

void Archive::reopen() {
  assert(!in_arc);
  open(get_archive_path(), format_chain);
}

void Archive::close() {
  if (in_arc) {
    in_arc->Close();
    in_arc.Release();
  }
  file_list.clear();
  file_list_index.clear();
}
