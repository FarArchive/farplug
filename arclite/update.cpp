#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"

wstring format_time(unsigned __int64 t) {
  unsigned __int64 s = t % 60;
  unsigned __int64 m = (t / 60) % 60;
  unsigned __int64 h = t / 60 / 60;
  wostringstream st;
  st << setfill(L'0') << setw(2) << h << L":" << setw(2) << m << L":" << setw(2) << s;
  return st.str();
}

class ArchiveUpdateProgress: public ProgressMonitor {
private:
  bool new_arc;
  UInt64 total;
  UInt64 completed;
  wstring file_path;
  UInt64 file_total;
  UInt64 file_completed;

  bool paused;
  bool low_priority;
  DWORD initial_priority;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    unsigned file_percent = calc_percent(file_completed, file_total);
    unsigned percent = calc_percent(completed, total);

    unsigned __int64 time = time_elapsed();
    unsigned __int64 speed;
    if (time == 0)
      speed = 0;
    else
      speed = round(static_cast<double>(completed) / time * ticks_per_sec());

    unsigned __int64 total_time;
    if (completed)
      total_time = static_cast<unsigned __int64>(static_cast<double>(total) / completed * time);
    else
      total_time = 0;
    if (total_time < time)
      total_time = time;

    st << Far::get_msg(new_arc ? MSG_PROGRESS_CREATE : MSG_PROGRESS_UPDATE) << L'\n';
    st << fit_str(file_path, c_width) << L'\n';
    st << setw(7) << format_data_size(file_completed, get_size_suffixes()) << L" / " << format_data_size(file_total, get_size_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(c_width, file_percent, 100) << L'\n';
    st << L"\x1\n";

    st << setw(7) << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L" [" << setw(2) << percent << L"%] @ " << setw(9) << format_data_size(speed, get_speed_suffixes()) << L" -" << format_time((total_time - time) / ticks_per_sec()) << L'\n';
    st << Far::get_progress_bar_str(c_width, percent, 100) << L'\n';
    st << L"\x1\n";

    wstring cmd_str;
    cmd_str += Far::get_msg(paused ? MSG_PROGRESS_UNPAUSE : MSG_PROGRESS_PAUSE);
    cmd_str += L", ";
    cmd_str += Far::get_msg(low_priority ? MSG_PROGRESS_NORMAL_PRIORITY : MSG_PROGRESS_LOW_PRIORITY);
    st << center(cmd_str, c_width) << L'\n';

    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_NORMAL);
    Far::set_progress_value(percent, 100);

    SetConsoleTitleW((L"{" + int_to_str(percent) + L"%} " + Far::get_msg(paused ? MSG_PROGRESS_PAUSED : new_arc ? MSG_PROGRESS_CREATE : MSG_PROGRESS_UPDATE)).c_str());
  }

  virtual void do_process_key(const KEY_EVENT_RECORD& key_event) {
    const WORD c_vk_b = 0x42;
    const WORD c_vk_p = 0x50;
    if (is_single_key(key_event)) {
      if (key_event.wVirtualKeyCode == c_vk_b) {
        low_priority = !low_priority;
        SetPriorityClass(GetCurrentProcess(), low_priority ? IDLE_PRIORITY_CLASS : initial_priority);
        do_update_ui();
      }
      else if (key_event.wVirtualKeyCode == c_vk_p) {
        paused = !paused;
        do_update_ui();
        if (paused) {
          HANDLE h_con = GetStdHandle(STD_INPUT_HANDLE);
          INPUT_RECORD rec;
          DWORD read_cnt;
          while (paused) {
            ReadConsoleInputW(h_con, &rec, 1, &read_cnt);
            if (rec.EventType == KEY_EVENT) {
              const KEY_EVENT_RECORD& ke = rec.Event.KeyEvent;
              if (is_single_key(ke)) {
                if (ke.wVirtualKeyCode == VK_ESCAPE) {
                  handle_esc();
                }
                else if (ke.wVirtualKeyCode == c_vk_p) {
                  paused = false;
                }
              }
            }
          }
        }
      }
    }
  }

public:
  ArchiveUpdateProgress(bool new_arc): ProgressMonitor(true), new_arc(new_arc), completed(0), total(0), file_completed(0), file_total(0), paused(false), low_priority(false) {
    initial_priority = GetPriorityClass(GetCurrentProcess());
    CHECK_SYS(initial_priority);
  }
  ~ArchiveUpdateProgress() {
    SetPriorityClass(GetCurrentProcess(), initial_priority);
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


DWORD translate_seek_method(UInt32 seek_origin) {
  DWORD method;
  switch (seek_origin) {
  case STREAM_SEEK_SET:
    method = FILE_BEGIN; break;
  case STREAM_SEEK_CUR:
    method = FILE_CURRENT; break;
  case STREAM_SEEK_END:
    method = FILE_END; break;
  default:
    FAIL(E_INVALIDARG);
  }
  return method;
}

class UpdateStream: public IOutStream {
public:
  virtual void clean_files() throw() = 0;
};


class SimpleUpdateStream: public UpdateStream, public ComBase, private File {
private:
  wstring file_path;

public:
  SimpleUpdateStream(const wstring& file_path): file_path(file_path) {
    open(file_path, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
  }
  ~SimpleUpdateStream() {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_ITF(IOutStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    unsigned size_written = write(data, size);
    if (processedSize)
      *processedSize = size_written;
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
    COM_ERROR_HANDLER_BEGIN
    unsigned __int64 new_position = set_pos(offset, translate_seek_method(seekOrigin));
    if (newPosition)
      *newPosition = new_position;
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetSize(UInt64 newSize) {
    COM_ERROR_HANDLER_BEGIN
    set_pos(newSize, FILE_BEGIN);
    set_end();
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  virtual void clean_files() throw() {
    close();
    DeleteFileW(long_path(file_path).c_str());
  }
};

IOutStream* get_simple_update_stream(const wstring& arc_path) {
  return new SimpleUpdateStream(arc_path);
}


class SfxUpdateStream: public UpdateStream, public ComBase, private File {
private:
  wstring file_path;
  unsigned __int64 start_offset;

  void write_sfx_header(unsigned sfx_module_idx) {
    const SfxModules& sfx_modules = ArcAPI::sfx();
    CHECK(sfx_module_idx < sfx_modules.size());
    wstring sfx_module_path = sfx_modules[sfx_module_idx].path;
    File sfx_module(sfx_module_path, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING, 0);
    unsigned __int64 sfx_module_size = sfx_module.size();
    CHECK(sfx_module_size < 1024 * 1024);
    Buffer<char> buffer(static_cast<size_t>(sfx_module_size));
    CHECK(sfx_module.read(buffer.data(), static_cast<unsigned>(buffer.size())) == sfx_module_size);
    CHECK(write(buffer.data(), static_cast<unsigned>(buffer.size())) == buffer.size());
    start_offset = buffer.size();
  }

public:
  SfxUpdateStream(const wstring& file_path, unsigned sfx_module_idx): file_path(file_path), start_offset(0) {
    open(file_path, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
    write_sfx_header(sfx_module_idx);
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_ITF(IOutStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    DWORD size_written;
    CHECK_SYS(WriteFile(h_file, data, size, &size_written, nullptr));
    if (processedSize)
      *processedSize = size_written;
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
    COM_ERROR_HANDLER_BEGIN
    __int64 real_offset = offset;
    if (seekOrigin == STREAM_SEEK_SET)
      real_offset += start_offset;
    unsigned __int64 new_position = set_pos(real_offset, translate_seek_method(seekOrigin));
    if (new_position < start_offset)
      FAIL(E_INVALIDARG);
    new_position -= start_offset;
    if (newPosition)
      *newPosition = new_position;
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetSize(UInt64 newSize) {
    COM_ERROR_HANDLER_BEGIN
    set_pos(newSize + start_offset);
    set_end();
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  virtual void clean_files() throw() {
    close();
    DeleteFileW(long_path(file_path).c_str());
  }
};


class MultiVolumeUpdateStream: public UpdateStream, public ComBase {
private:
  wstring file_path;
  unsigned __int64 volume_size;

  unsigned __int64 stream_pos;
  unsigned __int64 seek_stream_pos;
  unsigned __int64 stream_size;
  bool next_volume;
  File volume;

  wstring get_volume_path(unsigned __int64 volume_idx) {
    wstring volume_ext = uint_to_str(volume_idx + 1);
    if (volume_ext.size() < 3)
      volume_ext.insert(0, 3 - volume_ext.size(), L'0');
    volume_ext.insert(0, 1, L'.');

    size_t pos = file_path.find_last_of(L'.');
    if (pos != wstring::npos && pos != 0) {
      wstring ext = file_path.substr(pos);
      if (_wcsicmp(ext.c_str(), c_volume_ext) == 0)
        return file_path.substr(0, pos) + volume_ext;
    }
    return file_path + volume_ext;
  }

  unsigned __int64 get_last_volume_idx() {
    return stream_size ? (stream_size - 1) / volume_size : 0;
  }

public:
  MultiVolumeUpdateStream(const wstring& file_path, unsigned __int64 volume_size): file_path(file_path), volume_size(volume_size), stream_pos(0), seek_stream_pos(0), stream_size(0), next_volume(false) {
    volume.open(get_volume_path(0), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_ITF(IOutStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    if (seek_stream_pos != stream_pos) {
      unsigned __int64 volume_idx = seek_stream_pos / volume_size;
      unsigned __int64 last_volume_idx = get_last_volume_idx();
      while (last_volume_idx + 1 < volume_idx) {
        last_volume_idx += 1;
        volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
        volume.set_pos(volume_size);
        volume.set_end();
      }
      if (last_volume_idx < volume_idx) {
        last_volume_idx += 1;
        assert(last_volume_idx == volume_idx);
        volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
      }
      else {
        volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING, 0);
      }
      volume.set_pos(seek_stream_pos - volume_idx * volume_size);
      stream_pos = seek_stream_pos;
      next_volume = false;
    }

    unsigned data_off = 0;
    do {
      unsigned __int64 volume_idx = stream_pos / volume_size;

      if (next_volume) { // advance to next volume
        if (volume_idx > get_last_volume_idx()) {
          volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
        }
        else {
          volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING, 0);
        }
      }

      unsigned __int64 volume_upper_bound = (volume_idx + 1) * volume_size;
      unsigned write_size;
      if (stream_pos + (size - data_off) >= volume_upper_bound) {
        write_size = static_cast<unsigned>(volume_upper_bound - stream_pos);
        next_volume = true;
      }
      else
        write_size = size - data_off;
      CHECK(volume.write(reinterpret_cast<const unsigned char*>(data) + data_off, write_size) == write_size);
      data_off += write_size;
      stream_pos += write_size;
      seek_stream_pos = stream_pos;
      if (stream_size < stream_pos)
        stream_size = stream_pos;
    }
    while (data_off < size);
    if (processedSize)
      *processedSize = size;
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
    COM_ERROR_HANDLER_BEGIN
    switch (seekOrigin) {
    case STREAM_SEEK_SET:
      seek_stream_pos = offset;
      break;
    case STREAM_SEEK_CUR:
      seek_stream_pos += offset;
      break;
    case STREAM_SEEK_END:
      if (offset < 0 && static_cast<unsigned>(-offset) > stream_size)
        FAIL(E_INVALIDARG);
      seek_stream_pos = stream_size + offset;
      break;
    default:
      FAIL(E_INVALIDARG);
    }
    if (newPosition)
      *newPosition = seek_stream_pos;
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP SetSize(UInt64 newSize) {
    COM_ERROR_HANDLER_BEGIN
    if (stream_size == newSize)
      return S_OK;

    unsigned __int64 last_volume_idx = get_last_volume_idx();
    unsigned __int64 volume_idx = static_cast<unsigned>(newSize / volume_size);
    while (last_volume_idx + 1 < volume_idx) {
      last_volume_idx += 1;
      volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
      volume.set_pos(volume_size);
      volume.set_end();
    }
    if (last_volume_idx < volume_idx) {
      last_volume_idx += 1;
      assert(last_volume_idx == volume_idx);
      volume.open(get_volume_path(last_volume_idx), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
    }
    else {
      volume.open(get_volume_path(volume_idx), GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING, 0);
    }
    volume.set_pos(newSize - volume_idx * volume_size);
    volume.set_end();

    for (unsigned __int64 extra_idx = volume_idx + 1; extra_idx <= last_volume_idx; extra_idx++) {
      CHECK_SYS(DeleteFileW(long_path(get_volume_path(extra_idx)).c_str()));
    }

    stream_size = newSize;

    return S_OK;
    COM_ERROR_HANDLER_END
  }

  virtual void clean_files() throw() {
    volume.close();
    unsigned __int64 last_volume_idx = get_last_volume_idx();
    for (unsigned __int64 volume_idx = 0; volume_idx <= last_volume_idx; volume_idx++) {
      DeleteFileW(long_path(get_volume_path(volume_idx)).c_str());
    }
  }
};


class FileReadStream: public IInStream, public ComBase, private File {
private:
  wstring file_path;
  ArchiveUpdateProgress& progress;

public:
  FileReadStream(const wstring& file_path, bool open_shared, ArchiveUpdateProgress& progress): file_path(file_path), progress(progress) {
    open(file_path, FILE_READ_DATA, FILE_SHARE_READ | (open_shared ? FILE_SHARE_WRITE | FILE_SHARE_DELETE : 0), OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialInStream)
  UNKNOWN_IMPL_ITF(IInStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    ERROR_MESSAGE_BEGIN
    unsigned bytes_read = read(data, size);
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
    unsigned __int64 new_position = set_pos(offset, translate_seek_method(seekOrigin));
    if (newPosition)
      *newPosition = new_position;
    return S_OK;
    ERROR_MESSAGE_END(file_path)
    COM_ERROR_HANDLER_END
  }
};


class ArchiveUpdater: public IArchiveUpdateCallback, public ICryptoGetTextPassword2, public ComBase {
private:
  wstring src_dir;
  wstring dst_dir;
  UInt32 num_indices;
  const FileIndexMap& file_index_map;
  const wstring& password;
  bool open_shared;
  bool& ignore_errors;
  ErrorLog& error_log;
  ArchiveUpdateProgress progress;

public:
  ArchiveUpdater(const wstring& src_dir, const wstring& dst_dir, UInt32 num_indices, const FileIndexMap& file_index_map, const wstring& password, bool open_shared, bool& ignore_errors, ErrorLog& error_log): src_dir(src_dir), dst_dir(dst_dir), num_indices(num_indices), file_index_map(file_index_map), password(password), open_shared(open_shared), progress(num_indices == 0), ignore_errors(ignore_errors), error_log(error_log) {
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
    *inStream = nullptr;
    const FileIndexInfo& file_index_info = file_index_map.at(index);

    if (file_index_info.find_data.is_dir())
      return S_OK;

    wstring file_path = add_trailing_slash(add_trailing_slash(src_dir) + file_index_info.rel_path) + file_index_info.find_data.cFileName;
    progress.on_open_file(file_path, file_index_info.find_data.size());

    FileReadStream* file_read_stream = nullptr;
    while (true) {
      try {
        file_read_stream = new FileReadStream(file_path, open_shared, progress);
        break;
      }
      catch (const Error& e) {
        if (retry_or_ignore_error(file_path, e, ignore_errors, error_log, progress))
          break;
      }
    }

    if (file_read_stream) {
      ComObject<ISequentialInStream> stream(file_read_stream);
      stream.detach(inStream);
      return S_OK;
    }
    else
      return S_FALSE;

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
    BStr(password).detach(pwd);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


class PrepareUpdateProgress: private ProgressMonitor {
private:
  const wstring* file_path;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    st << Far::get_msg(MSG_PROGRESS_SCAN_DIRS) << L'\n';
    st << left << setw(c_width) << fit_str(*file_path, c_width) << L'\n';

    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_INDETERMINATE);

    SetConsoleTitleW(Far::get_msg(MSG_PROGRESS_SCAN_DIRS).c_str());
  }

public:
  PrepareUpdateProgress(): ProgressMonitor(true) {
  }
  void update(const wstring& file_path) {
    this->file_path = &file_path;
    update_ui();
  }
};

UInt32 Archive::scan_file(const wstring& sub_dir, const FindData& src_find_data, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map, PrepareUpdateProgress& progress) {
  ArcFileInfo file_info;
  file_info.attr = src_find_data.is_dir() ? FILE_ATTRIBUTE_DIRECTORY : 0;
  file_info.parent = dst_dir_index;
  file_info.name = src_find_data.cFileName;
  FileIndexRange fi_range = equal_range(file_list_index.begin(), file_list_index.end(), -1, [&] (UInt32 left, UInt32 right) -> bool {
    const ArcFileInfo& fi_left = left == -1 ? file_info : file_list[left];
    const ArcFileInfo& fi_right = right == -1 ? file_info : file_list[right];
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

void Archive::scan_dir(const wstring& src_dir, const wstring& sub_dir, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map, PrepareUpdateProgress& progress) {
  wstring path = add_trailing_slash(src_dir) + sub_dir;
  progress.update(path);
  FileEnum file_enum(path);
  while (file_enum.next()) {
    UInt32 file_index = scan_file(sub_dir, file_enum.data(), dst_dir_index, new_index, file_index_map, progress);
    if (file_enum.data().is_dir()) {
      scan_dir(src_dir, add_trailing_slash(sub_dir) + file_enum.data().cFileName, file_index, new_index, file_index_map, progress);
    }
  }
}

void Archive::prepare_file_index_map(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map) {
  PrepareUpdateProgress progress;
  for (unsigned i = 0; i < items_number; i++) {
    const FAR_FIND_DATA& find_data = panel_items[i].FindData;
    UInt32 file_index = scan_file(wstring(), get_find_data(add_trailing_slash(src_dir) + find_data.lpwszFileName), dst_dir_index, new_index, file_index_map, progress);
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      scan_dir(src_dir, find_data.lpwszFileName, file_index, new_index, file_index_map, progress);
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
    if (options.arc_type == c_guid_7z) {
      if (options.level != 0) {
        names.push_back(L"0");
        values.push_back(options.method);
        names.push_back(L"s");
        values.push_back(options.solid);
      }
      if (options.encrypt) {
        if (options.encrypt_header_defined) {
          names.push_back(L"he");
          values.push_back(options.encrypt_header);
        }
      }
    }
    CHECK_COM(set_props->SetProperties(names.data(), values.data(), static_cast<Int32>(names.size())));
  }
}

class DeleteFilesProgress: public ProgressMonitor {
private:
  const wstring* file_path;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    st << Far::get_msg(MSG_PROGRESS_DELETE_FILES) << L'\n';
    st << left << setw(c_width) << fit_str(*file_path, c_width) << L'\n';
    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_INDETERMINATE);

    SetConsoleTitleW(Far::get_msg(MSG_PROGRESS_DELETE_FILES).c_str());
  }

public:
  DeleteFilesProgress(): ProgressMonitor(true) {
  }
  void update(const wstring& file_path) {
    this->file_path = &file_path;
    update_ui();
  }
};

void Archive::delete_src_file(const wstring& file_path, DeleteFilesProgress& progress) {
  progress.update(file_path);
  ERROR_MESSAGE_BEGIN
  if (!DeleteFileW(long_path(file_path).c_str())) {
    CHECK_SYS(GetLastError() == ERROR_ACCESS_DENIED);
    SetFileAttributesW(long_path(file_path).c_str(), FILE_ATTRIBUTE_NORMAL);
    CHECK_SYS(DeleteFileW(long_path(file_path).c_str()));
  }
  ERROR_MESSAGE_END(file_path)
}

void Archive::delete_src_dir(const wstring& dir_path, DeleteFilesProgress& progress) {
  {
    FileEnum file_enum(dir_path);
    while (file_enum.next()) {
      wstring path = add_trailing_slash(dir_path) + file_enum.data().cFileName;
      progress.update(path);
      if (file_enum.data().is_dir())
        delete_src_dir(path, progress);
      else
        delete_src_file(path, progress);
    }
  }
  ERROR_MESSAGE_BEGIN
  if (!RemoveDirectoryW(long_path(dir_path).c_str())) {
    CHECK_SYS(GetLastError() == ERROR_ACCESS_DENIED);
    SetFileAttributesW(long_path(dir_path).c_str(), FILE_ATTRIBUTE_NORMAL);
    CHECK_SYS(RemoveDirectoryW(long_path(dir_path).c_str()));
  }
  ERROR_MESSAGE_END(dir_path)
}

void Archive::delete_src_files(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number) {
  DeleteFilesProgress delete_files_progress;
  for (unsigned i = 0; i < items_number; i++) {
    const FAR_FIND_DATA& find_data = panel_items[i].FindData;
    wstring file_path = add_trailing_slash(src_dir) + find_data.lpwszFileName;
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      delete_src_dir(file_path, delete_files_progress);
    else
      delete_src_file(file_path, delete_files_progress);
  }
}

void Archive::create(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const UpdateOptions& options, ErrorLog& error_log) {
  bool ignore_errors = options.ignore_errors;

  FileIndexMap file_index_map;
  UInt32 new_index = 0;
  prepare_file_index_map(src_dir, panel_items, items_number, c_root_index, new_index, file_index_map);

  ComObject<IOutArchive> out_arc;
  ArcAPI::create_out_archive(options.arc_type, &out_arc);

  set_properties(out_arc, options);

  ComObject<IArchiveUpdateCallback> updater(new ArchiveUpdater(src_dir, wstring(), 0, file_index_map, options.password, options.open_shared, ignore_errors, error_log));
  UpdateStream* stream_impl;
  if (options.enable_volumes)
    stream_impl = new MultiVolumeUpdateStream(options.arc_path, parse_size_string(options.volume_size));
  else if (options.create_sfx && options.arc_type == c_guid_7z)
    stream_impl = new SfxUpdateStream(options.arc_path, options.sfx_module_idx);
  else
    stream_impl = new SimpleUpdateStream(options.arc_path);
  ComObject<IOutStream> update_stream(stream_impl);

  try {
    COM_ERROR_CHECK(out_arc->UpdateItems(update_stream, new_index, updater));
  }
  catch (...) {
    stream_impl->clean_files();
    throw;
  }

  if (options.move_files && error_log.empty())
    delete_src_files(src_dir, panel_items, items_number);
}

void Archive::update(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const wstring& dst_dir, const UpdateOptions& options, ErrorLog& error_log) {
  bool ignore_errors = options.ignore_errors;

  FileIndexMap file_index_map;
  UInt32 new_index = num_indices; // starting index for new files
  prepare_file_index_map(src_dir, panel_items, items_number, find_dir(dst_dir), new_index, file_index_map);

  wstring temp_arc_name = get_temp_file_name();
  try {
    ComObject<IOutArchive> out_arc;
    CHECK_COM(in_arc->QueryInterface(IID_IOutArchive, reinterpret_cast<void**>(&out_arc)));

    set_properties(out_arc, options);

    ComObject<IArchiveUpdateCallback> updater(new ArchiveUpdater(src_dir, dst_dir, num_indices, file_index_map, options.password, options.open_shared, ignore_errors, error_log));
    ComObject<IOutStream> update_stream(new SimpleUpdateStream(temp_arc_name));

    COM_ERROR_CHECK(out_arc->UpdateItems(update_stream, new_index, updater));
    close();
    update_stream.Release();
    CHECK_SYS(MoveFileExW(long_path(temp_arc_name).c_str(), long_path(arc_path).c_str(), MOVEFILE_REPLACE_EXISTING));
  }
  catch (...) {
    DeleteFileW(long_path(temp_arc_name).c_str());
    throw;
  }

  reopen();

  if (options.move_files && error_log.empty())
    delete_src_files(src_dir, panel_items, items_number);
}
