#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

class FileUpdateStream: public IOutStream, public UnknownImpl {
private:
  HANDLE h_file;
  const wstring& file_path;
  Error& error;
public:
  FileUpdateStream(const wstring& file_path, Error& error);
  ~FileUpdateStream();
  UNKNOWN_DECL
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(Int64 newSize);
};


class ArchiveUpdater: public IArchiveUpdateCallback, public ProgressMonitor, public UnknownImpl {
private:
  Archive& archive;
  wstring src_dir;
  wstring dst_dir;
  struct FileIndexInfo {
    wstring rel_path;
    FindData find_data;
  };
  typedef map<UInt32, FileIndexInfo> FileIndexMap;
  FileIndexMap file_index_map;
  Error error;

  UInt64 total;
  UInt64 completed;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
    unsigned percent;
    if (total == 0) percent = 0;
    else percent = static_cast<unsigned>(static_cast<double>(completed) * 100 / total);
    unsigned __int64 speed;
    if (time_elapsed() == 0) speed = 0;
    else speed = static_cast<unsigned>(static_cast<double>(completed) / time_elapsed() * ticks_per_sec());
    st << Far::get_msg(MSG_PROGRESS_UPDATE) << L'\n';
    st << setw(7) << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L" @ " << setw(9) << format_data_size(speed, get_speed_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(c_width, percent, 100) << L'\n';
    Far::set_progress_state(TBPF_NORMAL);
    Far::set_progress_value(percent, 100);
    SetConsoleTitleW((L"{" + int_to_str(percent) + L"%} " + Far::get_msg(MSG_PROGRESS_UPDATE)).c_str());
    Far::message(st.str(), 0, FMSG_LEFTALIGN);
  }

  class FileInStream: public ISequentialInStream, public UnknownImpl {
  private:
    HANDLE h_file;
    wstring file_path;
    Error& error;

  public:
    FileInStream(const wstring& file_path, Error& error): file_path(file_path), error(error) {
      h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
      CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
    }
    ~FileInStream() {
      CloseHandle(h_file);
    }

    UNKNOWN_IMPL_BEGIN
    UNKNOWN_IMPL_ITF(ISequentialInStream)
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
  };

public:
  ArchiveUpdater(Archive& archive, const wstring& src_dir, const wstring& dst_dir): archive(archive), src_dir(src_dir), dst_dir(dst_dir), completed(0), total(0) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IProgress)
  UNKNOWN_IMPL_ITF(IArchiveUpdateCallback)
  UNKNOWN_IMPL_END

  STDMETHODIMP SetTotal(UInt64 total) {
    COM_ERROR_HANDLER_BEGIN
    this->total = total;
    update_ui();
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetCompleted(const UInt64 *completeValue) {
    COM_ERROR_HANDLER_BEGIN
    if (completeValue) this->completed = *completeValue;
    update_ui();
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
      *indexInArchive = found->first < archive.num_indices ? found->first : -1;
    }
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value) {
    COM_ERROR_HANDLER_BEGIN
    const FileIndexInfo& file_index_info = file_index_map.at(index);
    PropVariant var;
    switch (propID) {
    case kpidPath:
      var = add_trailing_slash(add_trailing_slash(dst_dir) + file_index_info.rel_path) + file_index_info.find_data.cFileName; break;
    case kpidName:
      var = file_index_info.find_data.cFileName; break;
    case kpidIsDir:
      var = file_index_info.find_data.is_dir(); break;
    case kpidSize:
      var = file_index_info.find_data.size(); break;
    case kpidAttrib:
      var = static_cast<UInt32>(file_index_info.find_data.dwFileAttributes); break;
    case kpidCTime:
      var = file_index_info.find_data.ftCreationTime; break;
    case kpidATime:
      var = file_index_info.find_data.ftLastAccessTime; break;
    case kpidMTime:
      var = file_index_info.find_data.ftLastWriteTime; break;
    }
    var.detach(value);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP GetStream(UInt32 index, ISequentialInStream **inStream) {
    COM_ERROR_HANDLER_BEGIN
    const FileIndexInfo& file_index_info = file_index_map.at(index);
    wstring file_path = add_trailing_slash(add_trailing_slash(src_dir) + file_index_info.rel_path) + file_index_info.find_data.cFileName;
    ComObject<ISequentialInStream> stream(new FileInStream(file_path, error));
    stream.detach(inStream);
    update_ui();
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetOperationResult(Int32 operationResult) {
    COM_ERROR_HANDLER_BEGIN
    return S_OK;
    COM_ERROR_HANDLER_END
  }

public:

  UInt32 scan_file(const wstring& sub_dir, const FindData& src_find_data, UInt32 dst_dir_index, UInt32& new_index) {
    FileInfo file_info;
    file_info.attr = src_find_data.is_dir() ? FILE_ATTRIBUTE_DIRECTORY : 0;
    file_info.parent = dst_dir_index;
    file_info.name = src_find_data.cFileName;
    FileIndexRange fi_range = equal_range(archive.file_list_index.begin(), archive.file_list_index.end(), -1, [&] (UInt32 left, UInt32 right) -> bool {
      const FileInfo& fi_left = left == -1 ? file_info : archive.file_list[left];
      const FileInfo& fi_right = right == -1 ? file_info : archive.file_list[right];
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
      if (file_index >= archive.num_indices) { // fake index
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

  void scan_dir(const wstring& src_dir, const wstring& sub_dir, UInt32 dst_dir_index, UInt32& new_index) {
    FileEnum file_enum(add_trailing_slash(src_dir) + sub_dir);
    while (file_enum.next()) {
      UInt32 file_index = scan_file(sub_dir, file_enum.data(), dst_dir_index, new_index);
      if (file_enum.data().is_dir()) {
        scan_dir(src_dir, add_trailing_slash(sub_dir) + file_enum.data().cFileName, file_index, new_index);
      }
    }
  }

  void update(const PluginPanelItem* panel_items, unsigned items_number, const wstring& arc_name, const wstring& arc_type) {
    UInt32 new_index = archive.num_indices;
    UInt32 dst_dir_index = archive.is_empty() ? c_root_index : archive.find_dir(dst_dir);
    for (unsigned i = 0; i < items_number; i++) {
      const FAR_FIND_DATA& find_data = panel_items[i].FindData;
      scan_file(wstring(), get_find_data(add_trailing_slash(src_dir) + find_data.lpwszFileName), dst_dir_index, new_index);
      if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        scan_dir(src_dir, find_data.lpwszFileName, dst_dir_index, new_index);
      }
    }

    ComObject<IOutArchive> out_arc;
    if (archive.is_empty()) {
      const ArcFormat* arc_format = ArcAPI::get()->find_format(arc_type);
      const ArcLib& arc_lib = ArcAPI::get()->libs()[arc_format->lib_index];
      CHECK(arc_format);
      CHECK_COM(arc_lib.CreateObject(reinterpret_cast<const GUID*>(arc_format->class_id.data()), &IID_IOutArchive, reinterpret_cast<void**>(&out_arc)));
    }
    else {
      CHECK_COM(archive.in_arc->QueryInterface(IID_IOutArchive, reinterpret_cast<void**>(&out_arc)));
    }

    ComObject<FileUpdateStream> update_stream(new FileUpdateStream(arc_name, error));
    HRESULT res = out_arc->UpdateItems(update_stream, new_index, this);
    if (error.code != NO_ERROR)
      throw error;
    CHECK_COM(res);
  }

};


void Archive::create(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const UpdateOptions& options) {
  num_indices = 0;
  formats.assign(1, *ArcAPI::get()->find_format(options.arc_type));
  archive_dir = extract_file_path(options.arc_path);
  wcscpy(archive_file_info.cFileName, extract_file_name(options.arc_path).c_str());
  ComObject<ArchiveUpdater> updater(new ArchiveUpdater(*this, src_dir, wstring()));
  updater->update(panel_items, items_number, get_file_name(), options.arc_type);
  reopen();
}

void Archive::update(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const wstring& dst_dir, const UpdateOptions& options) {
  wstring temp_arc_name = get_temp_file_name();
  try {
    ComObject<ArchiveUpdater> updater(new ArchiveUpdater(*this, src_dir, dst_dir));
    updater->update(panel_items, items_number, temp_arc_name, options.arc_type);
    close();
    CHECK_SYS(MoveFileExW(temp_arc_name.c_str(), get_file_name().c_str(), MOVEFILE_REPLACE_EXISTING));
  }
  catch (...) {
    DeleteFileW(temp_arc_name.c_str());
    throw;
  }
  reopen();
}
