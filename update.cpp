#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

class ArchiveUpdater: public IArchiveUpdateCallback, public UnknownImpl, public ProgressMonitor {
private:
  Archive& archive;
  const vector<UInt32>& src_indices;
  const wstring& dst_file_name;
  vector<UInt32> new_indices;
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

  class FileUpdateStream: public IOutStream, public UnknownImpl {
  private:
    ArchiveUpdater& updater;
    HANDLE h_file;
    const wstring& file_path;
  public:
    FileUpdateStream(ArchiveUpdater& updater, const wstring& file_path): updater(updater), h_file(INVALID_HANDLE_VALUE), file_path(file_path) {
      h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
      CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
    }
    ~FileUpdateStream() {
      if (h_file != INVALID_HANDLE_VALUE)
        CloseHandle(h_file);
    }

    UNKNOWN_IMPL_BEGIN
    UNKNOWN_IMPL_ITF(ISequentialOutStream)
    UNKNOWN_IMPL_ITF(IOutStream)
    UNKNOWN_IMPL_END

    STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
      Error& error = updater.error;
      COM_ERROR_HANDLER_BEGIN
      CHECK_SYS(WriteFile(h_file, data, size, reinterpret_cast<LPDWORD>(processedSize), NULL));
      return S_OK;
      COM_ERROR_HANDLER_END;
    }

    STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
      Error& error = updater.error;
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
      Error& error = updater.error;
      COM_ERROR_HANDLER_BEGIN
      LARGE_INTEGER position;
      position.QuadPart = newSize;
      CHECK_SYS(SetFilePointerEx(h_file, position, NULL, FILE_BEGIN));
      CHECK_SYS(SetEndOfFile(h_file));
      return S_OK;
      COM_ERROR_HANDLER_END;
    }
  };

public:
  ArchiveUpdater(Archive& archive, const vector<UInt32>& src_indices, const wstring& dst_file_name): archive(archive), src_indices(src_indices), dst_file_name(dst_file_name), completed(0), total(0) {
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
    *newData = 0;
    *newProperties = 0;
    *indexInArchive = new_indices[index];
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value) {
    COM_ERROR_HANDLER_BEGIN
    PropVariant var;
    var.detach(value);
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

private:
  void enum_deleted_indices(UInt32 file_index, vector<UInt32>& indices) {
    const FileInfo& file_info = archive.file_list[file_index];
    if (file_info.is_dir()) {
      FileIndexRange dir_list = archive.get_dir_list(file_index);
      for_each(dir_list.first, dir_list.second, [&] (UInt32 file_index) {
        enum_deleted_indices(file_index, indices);
      });
    }
    else {
      indices.push_back(file_index);
    }
  }

public:

  void delete_files() {
    vector<UInt32> deleted_indices;
    deleted_indices.reserve(archive.file_list.size());
    for_each(src_indices.begin(), src_indices.end(), [&] (UInt32 src_index) {
      enum_deleted_indices(src_index, deleted_indices);
    });
    sort(deleted_indices.begin(), deleted_indices.end());
    vector<UInt32> file_indices;
    file_indices.reserve(archive.num_indices);
    for(UInt32 i = 0; i < archive.num_indices; i++) file_indices.push_back(i);
    new_indices.reserve(archive.num_indices);
    set_difference(file_indices.begin(), file_indices.end(), deleted_indices.begin(), deleted_indices.end(), back_insert_iterator<vector<UInt32>>(new_indices));
    ComObject<FileUpdateStream> update_stream(new FileUpdateStream(*this, dst_file_name));
    const ArcFormat& format = archive.formats.back();
    ComObject<IOutArchive> out_arc;
    CHECK_COM(archive.in_arc->QueryInterface(IID_IOutArchive, reinterpret_cast<void**>(&out_arc)));
    HRESULT res = out_arc->UpdateItems(update_stream, new_indices.size(), this);
    if (error.code != NO_ERROR)
      throw error;
    CHECK_COM(res);
  }
};

void Archive::delete_files(const vector<UInt32>& src_indices, const wstring& dst_file_name) {
  ComObject<ArchiveUpdater> updater(new ArchiveUpdater(*this, src_indices, dst_file_name));
  updater->delete_files();
}

wstring Archive::get_temp_file_name() const {
  GUID guid;
  CHECK_COM(CoCreateGuid(&guid));
  wchar_t guid_str[50];
  CHECK(StringFromGUID2(guid, guid_str, ARRAYSIZE(guid_str)));
  return add_trailing_slash(archive_dir) + guid_str + L".tmp";
}
