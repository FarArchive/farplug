#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "msearch.hpp"
#include "archive.hpp"

class ArchiveOpenStream: public IInStream, public ComBase {
private:
  HANDLE h_file;
  wstring file_path;
public:
  ArchiveOpenStream(const wstring& file_path, Error& error): ComBase(error), file_path(file_path) {
    h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
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
    CHECK_SYS(ReadFile(h_file, data, size, &bytes_read, nullptr));
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


class ArchiveOpener: public IArchiveOpenCallback, public IArchiveOpenVolumeCallback, public ICryptoGetTextPassword, public ComBase, public ProgressMonitor {
private:
  const wstring& archive_dir;
  FindData volume_file_info;
  wstring& password;

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

    unsigned percent;
    if (total_files)
      percent = calc_percent(completed_files, total_files);
    else
      percent = calc_percent(completed_bytes, total_bytes);

    Far::set_progress_state(TBPF_NORMAL);
    Far::set_progress_value(percent, 100);

    SetConsoleTitleW((L"{" + int_to_str(percent) + L"%} " + Far::get_msg(MSG_PROGRESS_OPEN)).c_str());
  }

public:
  ArchiveOpener(const wstring& archive_dir, const FindData& archive_file_info, wstring& password, Error& error): ComBase(error), archive_dir(archive_dir), volume_file_info(archive_file_info), password(password), total_files(0), total_bytes(0), completed_files(0), completed_bytes(0) {
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
    PropVariant prop;
    switch (propID) {
    case kpidName:
      prop = volume_file_info.cFileName; break;
    case kpidIsDir:
      prop = volume_file_info.is_dir(); break;
    case kpidSize:
      prop = volume_file_info.size(); break;
    case kpidAttrib:
      prop = static_cast<UInt32>(volume_file_info.dwFileAttributes); break;
    case kpidCTime:
      prop = volume_file_info.ftCreationTime; break;
    case kpidATime:
      prop = volume_file_info.ftLastAccessTime; break;
    case kpidMTime:
      prop = volume_file_info.ftLastWriteTime; break;
    }
    prop.detach(value);
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
    BStr(password).detach(pwd);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


bool Archive::open_sub_stream(IInArchive* in_arc, IInStream** sub_stream, wstring& sub_ext) {
  UInt32 main_subfile;
  PropVariant prop;
  if (in_arc->GetArchiveProperty(kpidMainSubfile, prop.ref()) != S_OK || prop.vt != VT_UI4)
    return false;
  main_subfile = prop.ulVal;

  UInt32 num_items;
  if (in_arc->GetNumberOfItems(&num_items) != S_OK || main_subfile >= num_items)
    return false;

  if (in_arc->GetProperty(main_subfile, kpidPath, prop.ref()) == S_OK && prop.vt == VT_BSTR)
    sub_ext = extract_file_ext(prop.bstrVal);

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
  CHECK_COM(in_stream->Seek(0, STREAM_SEEK_SET, nullptr));

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

void Archive::detect(IInStream* in_stream, const wstring& ext, bool all, ArchiveHandles& archive_handles) {
  ArcChain parent_chain;
  if (!archive_handles.empty())
    parent_chain = archive_handles.back().arc_chain;

  const ArcFormats& arc_formats = ArcAPI::formats();
  list<ArcEntry> arc_entries;
  set<ArcType> found_types;

  // 1. find formats by signature
  vector<string> signatures;
  signatures.reserve(arc_formats.size());
  vector<ArcType> sig_types;
  sig_types.reserve(arc_formats.size());
  for_each(arc_formats.begin(), arc_formats.end(), [&] (const pair<ArcType, ArcFormat>& arc_format) {
    if (!arc_format.second.start_signature.empty()) {
      sig_types.push_back(arc_format.first);
      signatures.push_back(arc_format.second.start_signature);
    }
  });

  Buffer<unsigned char> buffer(max_check_size);
  UInt32 size;
  CHECK_COM(in_stream->Read(buffer.data(), static_cast<UInt32>(buffer.size()), &size));

  vector<StrPos> sig_positions = msearch(buffer.data(), size, signatures);

  for_each(sig_positions.begin(), sig_positions.end(), [&] (const StrPos& sig_pos) {
    found_types.insert(sig_types[sig_pos.idx]);
    arc_entries.push_back(ArcEntry(sig_types[sig_pos.idx], sig_pos.pos));
  });

  // 2. find formats by file extension
  ArcTypes types_by_ext = arc_formats.find_by_ext(ext);
  for_each(types_by_ext.begin(), types_by_ext.end(), [&] (const ArcType& arc_type) {
    if (found_types.count(arc_type) == 0) {
      found_types.insert(arc_type);
      arc_entries.push_front(ArcEntry(arc_type, 0));
    }
  });

  // 3. all other formats
  for_each(arc_formats.begin(), arc_formats.end(), [&] (const pair<ArcType, ArcFormat>& arc_format) {
    if (found_types.count(arc_format.first) == 0) {
      arc_entries.push_back(ArcEntry(arc_format.first, 0));
    }
  });

  // special case: UDF must go before ISO
  list<ArcEntry>::iterator iso_iter = arc_entries.end();
  for (list<ArcEntry>::iterator arc_entry = arc_entries.begin(); arc_entry != arc_entries.end(); arc_entry++) {
    if (arc_entry->type == c_guid_iso) {
      iso_iter = arc_entry;
    }
    if (arc_entry->type == c_guid_udf) {
      if (iso_iter != arc_entries.end()) {
        arc_entries.insert(iso_iter, *arc_entry);
        arc_entries.erase(arc_entry);
      }
      break;
    }
  }

  for (list<ArcEntry>::const_iterator arc_entry = arc_entries.begin(); arc_entry != arc_entries.end(); arc_entry++) {
    ComObject<IInArchive> archive;
    ArcAPI::create_in_archive(arc_entry->type, &archive);
    if (open_archive(in_stream, archive)) {
      ArchiveHandle new_archive_handle;
      new_archive_handle.arc_chain.assign(parent_chain.begin(), parent_chain.end());
      new_archive_handle.arc_chain.push_back(*arc_entry);
      new_archive_handle.in_arc = archive;
      archive_handles.push_back(new_archive_handle);

      ComObject<IInStream> sub_stream;
      wstring sub_ext;
      if (open_sub_stream(archive, &sub_stream, sub_ext))
        detect(sub_stream, sub_ext, all, archive_handles);

      if (!all) break;
    }
  }
}

ArchiveHandles Archive::detect(const wstring& file_path, bool all) {
  archive_file_info = get_find_data(file_path);
  archive_dir = extract_file_path(file_path);
  ArchiveHandles archive_handles;
  Error error;
  ComObject<IInStream> stream(new ArchiveOpenStream(get_archive_path(), error));
  detect(stream, extract_file_ext(file_path), all, archive_handles);
  if (!all && !archive_handles.empty())
    archive_handles.erase(archive_handles.begin(), archive_handles.end() - 1);
  return archive_handles;
}

void Archive::open(const ArchiveHandle& archive_handle) {
  in_arc = archive_handle.in_arc;
  arc_chain = archive_handle.arc_chain;
}

void Archive::reopen() {
  assert(!in_arc);
  Error error;
  ComObject<IInStream> stream(new ArchiveOpenStream(get_archive_path(), error));
  ArcChain::const_iterator arc_entry = arc_chain.begin();
  ArcAPI::create_in_archive(arc_entry->type, &in_arc);
  if (!open_archive(stream, in_arc))
    FAIL(E_FAIL);
  arc_entry++;
  while (arc_entry != arc_chain.end()) {
    ComObject<IInStream> sub_stream;
    wstring sub_ext;
    CHECK(open_sub_stream(in_arc, &sub_stream, sub_ext));
    ArcAPI::create_in_archive(arc_entry->type, &in_arc);
    if (!open_archive(sub_stream, in_arc))
      FAIL(E_FAIL);
    arc_entry++;
  }
}

void Archive::close() {
  if (in_arc) {
    in_arc->Close();
    in_arc.Release();
  }
  file_list.clear();
  file_list_index.clear();
}
