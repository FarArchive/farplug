#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

class ArchiveUpdateStream: public IOutStream, public ComBase {
private:
  HANDLE h_file;
  const wstring& file_path;
public:
  ArchiveUpdateStream(const wstring& file_path, Error& error);
  ~ArchiveUpdateStream();
  UNKNOWN_DECL
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(Int64 newSize);
};

class ArchiveFileDeleter: public IArchiveUpdateCallback, public ProgressMonitor, public ComBase {
private:
  vector<UInt32> new_indices;

  UInt64 total;
  UInt64 completed;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;

    unsigned percent;
    if (total == 0)
      percent = 0;
    else
      percent = round(static_cast<double>(completed) * 100 / total);
    if (percent > 100)
      percent = 100;

    unsigned __int64 speed;
    if (time_elapsed() == 0)
      speed = 0;
    else
      speed = round(static_cast<double>(completed) / time_elapsed() * ticks_per_sec());

    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
    st << Far::get_msg(MSG_PROGRESS_UPDATE) << L'\n';
    st << setw(7) << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L" @ " << setw(9) << format_data_size(speed, get_speed_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(c_width, percent, 100) << L'\n';
    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_NORMAL);
    Far::set_progress_value(percent, 100);

    SetConsoleTitleW((L"{" + int_to_str(percent) + L"%} " + Far::get_msg(MSG_PROGRESS_UPDATE)).c_str());
  }

public:
  ArchiveFileDeleter(const vector<UInt32>& new_indices, Error& error): ComBase(error), new_indices(new_indices), completed(0), total(0) {
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
    if (completeValue) completed = *completeValue;
    update_ui();
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP GetUpdateItemInfo(UInt32 index, Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive) {
    COM_ERROR_HANDLER_BEGIN
    *newData = 0;
    *newProperties = 0;
    *indexInArchive = new_indices[index];
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value) {
    COM_ERROR_HANDLER_BEGIN
    PropVariant prop;
    prop.detach(value);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP GetStream(UInt32 index, ISequentialInStream **inStream) {
    COM_ERROR_HANDLER_BEGIN
    *inStream = nullptr;
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetOperationResult(Int32 operationResult) {
    COM_ERROR_HANDLER_BEGIN
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


void Archive::enum_deleted_indices(UInt32 file_index, vector<UInt32>& indices) {
  const FileInfo& file_info = file_list[file_index];
  indices.push_back(file_index);
  if (file_info.is_dir()) {
    FileIndexRange dir_list = get_dir_list(file_index);
    for_each(dir_list.first, dir_list.second, [&] (UInt32 file_index) {
      enum_deleted_indices(file_index, indices);
    });
  }
}

void Archive::delete_files(const vector<UInt32>& src_indices) {
  vector<UInt32> deleted_indices;
  deleted_indices.reserve(file_list.size());
  for_each(src_indices.begin(), src_indices.end(), [&] (UInt32 src_index) {
    enum_deleted_indices(src_index, deleted_indices);
  });
  sort(deleted_indices.begin(), deleted_indices.end());

  vector<UInt32> file_indices;
  file_indices.reserve(num_indices);
  for(UInt32 i = 0; i < num_indices; i++)
    file_indices.push_back(i);

  vector<UInt32> new_indices;
  new_indices.reserve(num_indices);
  set_difference(file_indices.begin(), file_indices.end(), deleted_indices.begin(), deleted_indices.end(), back_insert_iterator<vector<UInt32>>(new_indices));

  wstring temp_arc_name = get_temp_file_name();
  try {
    ComObject<IOutArchive> out_arc;
    CHECK_COM(in_arc->QueryInterface(IID_IOutArchive, reinterpret_cast<void**>(&out_arc)));

    Error error;
    ComObject<IArchiveUpdateCallback> deleter(new ArchiveFileDeleter(new_indices, error));
    ComObject<IOutStream> update_stream(new ArchiveUpdateStream(temp_arc_name, error));

    HRESULT res = out_arc->UpdateItems(update_stream, new_indices.size(), deleter);
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
