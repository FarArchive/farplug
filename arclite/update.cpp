#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

class ArchiveUpdateProgress: public ProgressMonitor {
private:
  bool new_arc;
  UInt64 total;
  UInt64 completed;
  wstring file_path;
  UInt64 file_total;
  UInt64 file_completed;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    unsigned file_percent;
    if (file_total == 0)
      file_percent = 0;
    else
      file_percent = round(static_cast<double>(file_completed) / file_total * 100);
    if (file_percent > 100)
      file_percent = 100;

    unsigned percent;
    if (total == 0)
      percent = 0;
    else
      percent = round(static_cast<double>(completed) / total * 100);
    if (percent > 100)
      percent = 100;

    unsigned __int64 speed;
    if (time_elapsed() == 0)
      speed = 0;
    else
      speed = round(static_cast<double>(completed) / time_elapsed() * ticks_per_sec());

    st << Far::get_msg(new_arc ? MSG_PROGRESS_CREATE : MSG_PROGRESS_UPDATE) << L'\n';
    st << fit_str(file_path, c_width) << L'\n';
    st << setw(7) << format_data_size(file_completed, get_size_suffixes()) << L" / " << format_data_size(file_total, get_size_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(c_width, file_percent, 100) << L'\n';
    st << L"\x1\n";

    st << setw(7) << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L" @ " << setw(9) << format_data_size(speed, get_speed_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(c_width, percent, 100) << L'\n';

    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_NORMAL);
    Far::set_progress_value(percent, 100);

    SetConsoleTitleW((L"{" + int_to_str(percent) + L"%} " + Far::get_msg(MSG_PROGRESS_UPDATE)).c_str());
  }

public:
  ArchiveUpdateProgress(bool new_arc): ProgressMonitor(true), new_arc(new_arc), completed(0), total(0), file_completed(0), file_total(0) {
  }

  void on_open_file(const wstring& file_path, unsigned __int64 size) {
    this->file_path = file_path;
    file_total = size;
    file_completed = 0;
    update_ui();
  }
  void on_read_file(unsigned size) {
    file_completed += size;
    update_ui();
  }
  void on_total_update(UInt64 total) {
    this->total = total;
    update_ui();
  }
  void on_completed_update(UInt64 completed) {
    this->completed = completed;
    update_ui();
  }

};


class ArchiveUpdateStream: public IOutStream, public UnknownImpl {
private:
  HANDLE h_file;
  const wstring& file_path;
  Error& error;

public:
  ArchiveUpdateStream(const wstring& file_path, Error& error): h_file(INVALID_HANDLE_VALUE), file_path(file_path), error(error) {
    h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
  }
  ~ArchiveUpdateStream() {
    if (h_file != INVALID_HANDLE_VALUE)
      CloseHandle(h_file);
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_ITF(IOutStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    CHECK_SYS(WriteFile(h_file, data, size, reinterpret_cast<LPDWORD>(processedSize), NULL));
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
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
    COM_ERROR_HANDLER_END;
  }
  STDMETHODIMP SetSize(Int64 newSize) {
    COM_ERROR_HANDLER_BEGIN
    LARGE_INTEGER position;
    position.QuadPart = newSize;
    CHECK_SYS(SetFilePointerEx(h_file, position, NULL, FILE_BEGIN));
    CHECK_SYS(SetEndOfFile(h_file));
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


class FileReadStream: public IInStream, public UnknownImpl {
private:
  HANDLE h_file;
  const wstring& file_path;
  ArchiveUpdateProgress& progress;
  Error& error;

public:
  FileReadStream(const wstring& file_path, ArchiveUpdateProgress& progress, Error& error): file_path(file_path), progress(progress), error(error) {
    h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
  }
  ~FileReadStream() {
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
    progress.on_read_file(bytes_read);
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


class ArchiveUpdater: public IArchiveUpdateCallback, public ICryptoGetTextPassword2, public UnknownImpl {
private:
  wstring src_dir;
  wstring dst_dir;
  UInt32 num_indices;
  const FileIndexMap& file_index_map;
  const wstring& password;
  ArchiveUpdateProgress progress;
  Error& error;

public:
  ArchiveUpdater(const wstring& src_dir, const wstring& dst_dir, UInt32 num_indices, const FileIndexMap& file_index_map, const wstring& password, Error& error): src_dir(src_dir), dst_dir(dst_dir), num_indices(num_indices), file_index_map(file_index_map), password(password), progress(num_indices == 0), error(error) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IProgress)
  UNKNOWN_IMPL_ITF(IArchiveUpdateCallback)
  UNKNOWN_IMPL_ITF(ICryptoGetTextPassword2)
  UNKNOWN_IMPL_END

  STDMETHODIMP SetTotal(UInt64 total) {
    COM_ERROR_HANDLER_BEGIN
    progress.on_total_update(total);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetCompleted(const UInt64 *completeValue) {
    COM_ERROR_HANDLER_BEGIN
    if (completeValue)
      progress.on_completed_update(*completeValue);
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP GetUpdateItemInfo(UInt32 index, Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive) {
    COM_ERROR_HANDLER_BEGIN
    FileIndexMap::const_iterator found = file_index_map.find(index);
    if (found == file_index_map.end()) {
      *newData = 0;
      *newProperties = 0;
      *indexInArchive = index;
    }
    else {
      *newData = 1;
      *newProperties = 1;
      *indexInArchive = found->first < num_indices ? found->first : -1;
    }
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value) {
    COM_ERROR_HANDLER_BEGIN
    const FileIndexInfo& file_index_info = file_index_map.at(index);
    PropVariant prop;
    switch (propID) {
    case kpidPath:
      prop = add_trailing_slash(add_trailing_slash(dst_dir) + file_index_info.rel_path) + file_index_info.find_data.cFileName; break;
    case kpidName:
      prop = file_index_info.find_data.cFileName; break;
    case kpidIsDir:
      prop = file_index_info.find_data.is_dir(); break;
    case kpidSize:
      prop = file_index_info.find_data.size(); break;
    case kpidAttrib:
      prop = static_cast<UInt32>(file_index_info.find_data.dwFileAttributes); break;
    case kpidCTime:
      prop = file_index_info.find_data.ftCreationTime; break;
    case kpidATime:
      prop = file_index_info.find_data.ftLastAccessTime; break;
    case kpidMTime:
      prop = file_index_info.find_data.ftLastWriteTime; break;
    }
    prop.detach(value);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP GetStream(UInt32 index, ISequentialInStream **inStream) {
    COM_ERROR_HANDLER_BEGIN
    const FileIndexInfo& file_index_info = file_index_map.at(index);
    wstring file_path = add_trailing_slash(add_trailing_slash(src_dir) + file_index_info.rel_path) + file_index_info.find_data.cFileName;
    progress.on_open_file(file_path, file_index_info.find_data.size());
    ComObject<ISequentialInStream> stream(new FileReadStream(file_path, progress, error));
    stream.detach(inStream);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetOperationResult(Int32 operationResult) {
    COM_ERROR_HANDLER_BEGIN
    if (operationResult == NArchive::NUpdate::NOperationResult::kError)
      FAIL_MSG(Far::get_msg(MSG_ERROR_UPDATE_ERROR));
    else
      return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *pwd) {
    COM_ERROR_HANDLER_BEGIN
    *passwordIsDefined = !password.empty();
    *pwd = str_to_bstr(password);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


UInt32 Archive::scan_file(const wstring& sub_dir, const FindData& src_find_data, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map) {
  FileInfo file_info;
  file_info.attr = src_find_data.is_dir() ? FILE_ATTRIBUTE_DIRECTORY : 0;
  file_info.parent = dst_dir_index;
  file_info.name = src_find_data.cFileName;
  FileIndexRange fi_range = equal_range(file_list_index.begin(), file_list_index.end(), -1, [&] (UInt32 left, UInt32 right) -> bool {
    const FileInfo& fi_left = left == -1 ? file_info : file_list[left];
    const FileInfo& fi_right = right == -1 ? file_info : file_list[right];
    return fi_left < fi_right;
  });
  UInt32 file_index;
  if (fi_range.first == fi_range.second) {
    // new file
    file_index = new_index;
    new_index++;
  }
  else {
    // updated file
    file_index = *fi_range.first;
    if (file_index >= num_indices) { // fake index
      file_index = new_index;
      new_index++;
    }
  }
  FileIndexInfo file_index_info;
  file_index_info.rel_path = sub_dir;
  file_index_info.find_data = src_find_data;
  file_index_map[file_index] = file_index_info;
  return file_index;
}

void Archive::scan_dir(const wstring& src_dir, const wstring& sub_dir, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map) {
  FileEnum file_enum(add_trailing_slash(src_dir) + sub_dir);
  while (file_enum.next()) {
    UInt32 file_index = scan_file(sub_dir, file_enum.data(), dst_dir_index, new_index, file_index_map);
    if (file_enum.data().is_dir()) {
      scan_dir(src_dir, add_trailing_slash(sub_dir) + file_enum.data().cFileName, file_index, new_index, file_index_map);
    }
  }
}

void Archive::prepare_file_index_map(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map) {
  for (unsigned i = 0; i < items_number; i++) {
    const FAR_FIND_DATA& find_data = panel_items[i].FindData;
    scan_file(wstring(), get_find_data(add_trailing_slash(src_dir) + find_data.lpwszFileName), dst_dir_index, new_index, file_index_map);
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      scan_dir(src_dir, find_data.lpwszFileName, dst_dir_index, new_index, file_index_map);
    }
  }
}

void Archive::set_properties(IOutArchive* out_arc, const UpdateOptions& options) {
  ComObject<ISetProperties> set_props;
  if (SUCCEEDED(out_arc->QueryInterface(IID_ISetProperties, reinterpret_cast<void**>(&set_props)))) {
    vector<const wchar_t*> names;
    vector<PropVariant> values;
    names.push_back(L"x");
    values.push_back(options.level);
    if (options.arc_type == L"7z") {
      names.push_back(L"0");
      values.push_back(options.method);
      names.push_back(L"s");
      values.push_back(options.solid);
      if (!options.password.empty()) {
        names.push_back(L"he");
        values.push_back(options.encrypt_header);
      }
    }
    CHECK_COM(set_props->SetProperties(names.data(), values.data(), names.size()));
  }
}

void Archive::create(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const UpdateOptions& options) {
  FileIndexMap file_index_map;
  UInt32 new_index = 0;
  prepare_file_index_map(src_dir, panel_items, items_number, c_root_index, new_index, file_index_map);

  try {
    ComObject<IOutArchive> out_arc;
    ArcAPI::get()->create_out_archive(ArcAPI::get()->find_format(options.arc_type), &out_arc);

    set_properties(out_arc, options);

    Error error;
    ComObject<IArchiveUpdateCallback> updater(new ArchiveUpdater(src_dir, wstring(), 0, file_index_map, options.password, error));
    ComObject<IOutStream> update_stream(new ArchiveUpdateStream(options.arc_path, error));

    HRESULT res = out_arc->UpdateItems(update_stream, new_index, updater);
    if (FAILED(res)) {
      if (error)
        throw error;
      else
        FAIL(res);
    }
  }
  catch (...) {
    DeleteFileW(long_path(options.arc_path).c_str());
    throw;
  }
}

void Archive::update(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const wstring& dst_dir, const UpdateOptions& options) {
  FileIndexMap file_index_map;
  UInt32 new_index = num_indices; // starting index for new files
  prepare_file_index_map(src_dir, panel_items, items_number, find_dir(dst_dir), new_index, file_index_map);

  wstring temp_arc_name = get_temp_file_name();
  try {
    ComObject<IOutArchive> out_arc;
    CHECK_COM(in_arc->QueryInterface(IID_IOutArchive, reinterpret_cast<void**>(&out_arc)));

    set_properties(out_arc, options);

    Error error;
    ComObject<IArchiveUpdateCallback> updater(new ArchiveUpdater(src_dir, dst_dir, num_indices, file_index_map, options.password, error));
    ComObject<IOutStream> update_stream(new ArchiveUpdateStream(temp_arc_name, error));

    HRESULT res = out_arc->UpdateItems(update_stream, new_index, updater);
    if (FAILED(res)) {
      if (error)
        throw error;
      else
        FAIL(res);
    }
    close();
    update_stream.Release();
    CHECK_SYS(MoveFileExW(temp_arc_name.c_str(), get_archive_path().c_str(), MOVEFILE_REPLACE_EXISTING));
  }
  catch (...) {
    DeleteFileW(long_path(temp_arc_name).c_str());
    throw;
  }

  reopen();
}
