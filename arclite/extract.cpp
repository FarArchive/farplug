#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"

bool retry_or_ignore_error(const wstring& path, const Error& error, bool& ignore_errors, ErrorLog& error_log, ProgressMonitor& progress) {
  if (error.code == E_ABORT)
    throw error;
  bool ignore = ignore_errors;
  if (!ignore) {
    ProgressSuspend ps(progress);
    switch (error_retry_ignore_dialog(path, error, true)) {
    case rdrRetry:
      break;
    case rdrIgnore:
      ignore = true;
      break;
    case rdrIgnoreAll:
      ignore = true;
      ignore_errors = true;
      break;
    case rdrCancel:
      FAIL(E_ABORT);
    }
  }
  if (ignore) {
    error_log.add(path, error);
    return true;
  }
  return false;
}

void ignore_error(const wstring& path, const Error& error, bool& ignore_errors, ErrorLog& error_log, ProgressMonitor& progress) {
  if (error.code == E_ABORT)
    throw error;
  if (!ignore_errors) {
    ProgressSuspend ps(progress);
    switch (error_retry_ignore_dialog(path, error, false)) {
    case rdrIgnore:
      break;
    case rdrIgnoreAll:
      ignore_errors = true;
      break;
    case rdrCancel:
      FAIL(E_ABORT);
    }
  }
  error_log.add(path, error);
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

wstring get_progress_bar_str(unsigned width, unsigned percent1, unsigned percent2) {
  const wchar_t c_pb_black = 9608;
  const wchar_t c_pb_gray = 9619;
  const wchar_t c_pb_white = 9617;
  unsigned len1 = round(static_cast<double>(percent1) / 100 * width);
  if (len1 > width)
    len1 = width;
  unsigned len2 = round(static_cast<double>(percent2) / 100 * width);
  if (len2 > width)
    len2 = width;
  if (len2 > len1)
    len2 -= len1;
  else
    len2 = 0;
  unsigned len3 = width - (len1 + len2);
  wstring result;
  result.append(len1, c_pb_black);
  result.append(len2, c_pb_gray);
  result.append(len3, c_pb_white);
  return result;
}

class ExtractProgress: public ProgressMonitor {
private:
  unsigned __int64 extract_completed;
  unsigned __int64 extract_total;
  wstring extract_file_path;
  unsigned __int64 cache_stored;
  unsigned __int64 cache_written;
  unsigned __int64 cache_total;
  wstring cache_file_path;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    unsigned extract_percent = calc_percent(extract_completed, extract_total);

    unsigned __int64 extract_speed;
    if (time_elapsed() == 0)
      extract_speed = 0;
    else
      extract_speed = round(static_cast<double>(extract_completed) / time_elapsed() * ticks_per_sec());

    if (extract_total && cache_total > extract_total)
      cache_total = extract_total;

    unsigned cache_stored_percent = calc_percent(cache_stored, cache_total);
    unsigned cache_written_percent = calc_percent(cache_written, cache_total);

    st << Far::get_msg(MSG_PROGRESS_EXTRACT) << L'\n';
    st << fit_str(extract_file_path, c_width) << L'\n';
    st << setw(7) << format_data_size(extract_completed, get_size_suffixes()) << L" / " << format_data_size(extract_total, get_size_suffixes()) << L" @ " << setw(9) << format_data_size(extract_speed, get_speed_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(c_width, extract_percent, 100) << L'\n';
    st << L"\x1\n";

    st << fit_str(cache_file_path, c_width) << L'\n';
    st << L"(" << format_data_size(cache_stored, get_size_suffixes()) << L" - " << format_data_size(cache_written, get_size_suffixes()) << L") / " << format_data_size(cache_total, get_size_suffixes()) << L'\n';
    st << get_progress_bar_str(c_width, cache_written_percent, cache_stored_percent) << L'\n';

    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_NORMAL);
    Far::set_progress_value(extract_percent, 100);

    SetConsoleTitleW((L"{" + int_to_str(extract_percent) + L"%} " + Far::get_msg(MSG_PROGRESS_EXTRACT)).c_str());
  }

public:
  ExtractProgress(): ProgressMonitor(true), extract_completed(0), extract_total(0), cache_stored(0), cache_written(0), cache_total(0) {
  }

  void update_extract_file(const wstring& file_path) {
    extract_file_path = file_path;
    update_ui();
  }
  void set_extract_total(unsigned __int64 size) {
    extract_total = size;
  }
  void update_extract_completed(unsigned __int64 size) {
    extract_completed = size;
    update_ui();
  }
  void update_cache_file(const wstring& file_path) {
    cache_file_path = file_path;
    update_ui();
  }
  void set_cache_total(unsigned __int64 size) {
    cache_total = size;
  }
  void update_cache_stored(unsigned __int64 size) {
    cache_stored += size;
    update_ui();
  }
  void update_cache_written(unsigned __int64 size) {
    cache_written += size;
    update_ui();
  }
  void reset_cache_stats() {
    cache_stored = 0;
    cache_written = 0;
  }
};


class FileWriteCache {
private:
  static const size_t c_min_cache_size = 10 * 1024 * 1024;
  static const size_t c_max_cache_size = 100 * 1024 * 1024;

  struct CacheRecord {
    wstring file_path;
    FileInfo file_info;
    size_t buffer_pos;
    size_t buffer_size;
  };

  unsigned char* buffer;
  size_t buffer_size;
  size_t commit_size;
  size_t buffer_pos;
  list<CacheRecord> cache_records;
  HANDLE h_file;
  CacheRecord current_rec;
  bool continue_file;
  bool error_state;
  bool& ignore_errors;
  ErrorLog& error_log;
  ExtractProgress& progress;

  size_t get_max_cache_size() const {
    MEMORYSTATUS mem_st;
    GlobalMemoryStatus(&mem_st);
    size_t size = mem_st.dwAvailPhys;
    if (size < c_min_cache_size)
      size = c_min_cache_size;
    if (size > c_max_cache_size)
      size = c_max_cache_size;
    return size;
  }
  // create new file
  void create_file() {
    while (true) {
      try {
        h_file = CreateFileW(long_path(current_rec.file_path).c_str(), FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
        CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
        break;
      }
      catch (const Error& e) {
        if (retry_or_ignore_error(current_rec.file_path, e, ignore_errors, error_log, progress)) {
          error_state = true;
          break;
        }
      }
    }
    progress.update_cache_file(current_rec.file_path);
  }
  // allocate file
  void allocate_file() {
    if (error_state) return;
    if (current_rec.file_info.size == 0) return;
    while (true) {
      try {
        LARGE_INTEGER file_pos;
        file_pos.QuadPart = current_rec.file_info.size;
        CHECK_SYS(SetFilePointerEx(h_file, file_pos, nullptr, FILE_BEGIN));
        CHECK_SYS(SetEndOfFile(h_file));
        file_pos.QuadPart = 0;
        CHECK_SYS(SetFilePointerEx(h_file, file_pos, nullptr, FILE_BEGIN));
        break;
      }
      catch (const Error& e) {
        if (retry_or_ignore_error(current_rec.file_path, e, ignore_errors, error_log, progress)) {
          error_state = true;
          break;
        }
      }
    }
  }
  // write file using 1 MB blocks
  void write_file() {
    if (error_state) return;
    try {
      const unsigned c_block_size = 1 * 1024 * 1024;
      size_t pos = 0;
      while (pos < current_rec.buffer_size) {
        DWORD size;
        if (pos + c_block_size > current_rec.buffer_size)
          size = static_cast<DWORD>(current_rec.buffer_size - pos);
        else
          size = c_block_size;
        DWORD size_written;
        CHECK_SYS(WriteFile(h_file, buffer + current_rec.buffer_pos + pos, size, &size_written, nullptr));
        pos += size_written;
        progress.update_cache_written(size_written);
      }
    }
    catch (const Error& e) {
      ignore_error(current_rec.file_path, e, ignore_errors, error_log, progress);
      error_state = true;
    }
  }
  void close_file() {
    if (h_file != INVALID_HANDLE_VALUE) {
      if (!error_state) {
        try {
          CHECK_SYS(SetEndOfFile(h_file)); // ensure end of file is set correctly
          CHECK_SYS(SetFileAttributesW(long_path(current_rec.file_path).c_str(), current_rec.file_info.attr));
          CHECK_SYS(SetFileTime(h_file, &current_rec.file_info.ctime, &current_rec.file_info.atime, &current_rec.file_info.mtime));
        }
        catch (const Error& e) {
          ignore_error(current_rec.file_path, e, ignore_errors, error_log, progress);
          error_state = true;
        }
      }
      CloseHandle(h_file);
      h_file = INVALID_HANDLE_VALUE;
      if (error_state)
        DeleteFileW(long_path(current_rec.file_path).c_str());
    }
  }
  void write() {
    for_each(cache_records.begin(), cache_records.end(), [&] (const CacheRecord& rec) {
      if (continue_file) {
        continue_file = false;
        current_rec = rec;
      }
      else {
        close_file();
        error_state = false; // reset error flag on each file
        current_rec = rec;
        create_file();
        allocate_file();
      }
      write_file();
    });
    // leave only last file record in cache
    if (!cache_records.empty()) {
      current_rec = cache_records.back();
      current_rec.buffer_pos = 0;
      current_rec.buffer_size = 0;
      cache_records.assign(1, current_rec);
      continue_file = true; // last file is not written in its entirety (possibly)
    }
    buffer_pos = 0;
    progress.reset_cache_stats();
  }
  void store(const unsigned char* data, size_t size) {
    assert(!cache_records.empty());
    assert(size <= buffer_size);
    if (buffer_pos + size > buffer_size) {
      write();
    }
    CacheRecord& rec = cache_records.back();
    size_t new_size = buffer_pos + size;
    if (new_size > commit_size) {
      CHECK_SYS(VirtualAlloc(buffer + commit_size, new_size - commit_size, MEM_COMMIT, PAGE_READWRITE));
      commit_size = new_size;
    }
    memcpy(buffer + buffer_pos, data, size);
    rec.buffer_size += size;
    buffer_pos += size;
    progress.update_cache_stored(size);
  }
public:
  FileWriteCache(bool& ignore_errors, ErrorLog& error_log, ExtractProgress& progress): buffer_size(get_max_cache_size()), commit_size(0), buffer_pos(0), h_file(INVALID_HANDLE_VALUE), continue_file(false), error_state(false), ignore_errors(ignore_errors), error_log(error_log), progress(progress) {
    progress.set_cache_total(buffer_size);
    buffer = reinterpret_cast<unsigned char*>(VirtualAlloc(nullptr, buffer_size, MEM_RESERVE, PAGE_NOACCESS));
    CHECK_SYS(buffer);
  }
  ~FileWriteCache() {
    VirtualFree(buffer, 0, MEM_RELEASE);
    if (h_file != INVALID_HANDLE_VALUE) {
      CloseHandle(h_file);
      DeleteFileW(long_path(current_rec.file_path).c_str());
    }
  }
  void store_file(const wstring& file_path, const FileInfo& file_info) {
    CacheRecord rec;
    rec.file_path = file_path;
    rec.file_info = file_info;
    rec.buffer_pos = buffer_pos;
    rec.buffer_size = 0;
    cache_records.push_back(rec);
  }
  void store_data(const unsigned char* data, size_t size) {
    unsigned full_buffer_cnt = static_cast<unsigned>(size / buffer_size);
    for (unsigned i = 0; i < full_buffer_cnt; i++) {
      store(data + i * buffer_size, buffer_size);
    }
    store(data + full_buffer_cnt * buffer_size, size % buffer_size);
  }
  void finalize() {
    write();
    close_file();
  }
};


class CachedFileExtractStream: public ISequentialOutStream, public ComBase {
private:
  FileWriteCache& cache;

public:
  CachedFileExtractStream(FileWriteCache& cache, Error& error): ComBase(error), cache(cache) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    cache.store_data(static_cast<const unsigned char*>(data), size);
    if (processedSize)
      *processedSize = size;
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};


class ArchiveExtractor: public IArchiveExtractCallback, public ICryptoGetTextPassword, public ComBase {
private:
  wstring file_path;
  FileInfo file_info;
  UInt32 src_dir_index;
  wstring dst_dir;
  const FileList& file_list;
  wstring& password;
  OverwriteOption& oo;
  bool& ignore_errors;
  ErrorLog& error_log;
  FileWriteCache& cache;
  ExtractProgress& progress;

public:
  ArchiveExtractor(UInt32 src_dir_index, const wstring& dst_dir, const FileList& file_list, wstring& password, OverwriteOption& oo, bool& ignore_errors, ErrorLog& error_log, Error& error, FileWriteCache& cache, ExtractProgress& progress): ComBase(error), src_dir_index(src_dir_index), dst_dir(dst_dir), file_list(file_list), password(password), oo(oo), ignore_errors(ignore_errors), error_log(error_log), cache(cache), progress(progress) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IProgress)
  UNKNOWN_IMPL_ITF(IArchiveExtractCallback)
  UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
  UNKNOWN_IMPL_END

  STDMETHODIMP SetTotal(UInt64 total) {
    COM_ERROR_HANDLER_BEGIN
    progress.set_extract_total(total);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetCompleted(const UInt64 *completeValue) {
    COM_ERROR_HANDLER_BEGIN
    if (completeValue)
      progress.update_extract_completed(*completeValue);
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP GetStream(UInt32 index, ISequentialOutStream **outStream,  Int32 askExtractMode) {
    COM_ERROR_HANDLER_BEGIN
    *outStream = nullptr;
    file_info = file_list[index];
    if (file_info.is_dir())
      return S_OK;

    file_path = file_info.name;
    UInt32 parent_index = file_info.parent;
    while (parent_index != src_dir_index && parent_index != c_root_index) {
      const FileInfo& file_info = file_list[parent_index];
      file_path.insert(0, 1, L'\\').insert(0, file_info.name);
      parent_index = file_info.parent;
    }
    file_path.insert(0, add_trailing_slash(dst_dir));

    if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
      return S_OK;

    FindData dst_file_info;
    HANDLE h_find = FindFirstFileW(long_path(file_path).c_str(), &dst_file_info);
    if (h_find != INVALID_HANDLE_VALUE) {
      FindClose(h_find);
      bool overwrite;
      if (oo == ooAsk) {
        FindData src_file_info = convert_file_info(file_info);
        ProgressSuspend ps(progress);
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
        SetFileAttributesW(long_path(file_path).c_str(), FILE_ATTRIBUTE_NORMAL);
      }
      else {
        *outStream = nullptr;
        return S_OK;
      }
    }

    progress.update_extract_file(file_path);
    cache.store_file(file_path, file_info);
    ComObject<ISequentialOutStream> out_stream(new CachedFileExtractStream(cache, error));
    out_stream.detach(outStream);

    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP PrepareOperation(Int32 askExtractMode) {
    COM_ERROR_HANDLER_BEGIN
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetOperationResult(Int32 resultEOperationResult) {
    COM_ERROR_HANDLER_BEGIN
    try {
      bool encrypted = !password.empty();
      if (resultEOperationResult == NArchive::NExtract::NOperationResult::kUnSupportedMethod) {
        FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_UNSUPPORTED_METHOD));
      }
      else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kDataError) {
        password.clear();
        FAIL_MSG(Far::get_msg(encrypted ? MSG_ERROR_EXTRACT_DATA_ERROR_ENCRYPTED : MSG_ERROR_EXTRACT_DATA_ERROR));
      }
      else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kCRCError) {
        password.clear();
        FAIL_MSG(Far::get_msg(encrypted ? MSG_ERROR_EXTRACT_CRC_ERROR_ENCRYPTED : MSG_ERROR_EXTRACT_CRC_ERROR));
      }
      else
        return S_OK;
    }
    catch (const Error& e) {
      ignore_error(file_path, e, ignore_errors, error_log, progress);
    }
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP CryptoGetTextPassword(BSTR *pwd) {
    COM_ERROR_HANDLER_BEGIN
    if (password.empty()) {
      ProgressSuspend ps(progress);
      if (!password_dialog(password))
        FAIL(E_ABORT);
    }
    BStr(password).detach(pwd);
    return S_OK;
    COM_ERROR_HANDLER_END
  }
};

void Archive::prepare_dst_dir(const wstring& path) {
  if (!is_root_path(path)) {
    prepare_dst_dir(extract_file_path(path));
    CreateDirectoryW(long_path(path).c_str(), nullptr);
  }
}

class PrepareExtractProgress: public ProgressMonitor {
private:
  const wstring* file_path;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    st << Far::get_msg(MSG_PROGRESS_CREATE_DIRS) << L'\n';
    st << left << setw(c_width) << fit_str(*file_path, c_width) << L'\n';

    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_INDETERMINATE);

    SetConsoleTitleW(Far::get_msg(MSG_PROGRESS_CREATE_DIRS).c_str());
  }

public:
  PrepareExtractProgress(): ProgressMonitor(true) {
  }
  void update(const wstring& file_path) {
    this->file_path = &file_path;
    update_ui();
  }
};

void Archive::prepare_extract(UInt32 file_index, const wstring& parent_dir, list<UInt32>& indices, bool& ignore_errors, ErrorLog& error_log, PrepareExtractProgress& progress) {
  const FileInfo& file_info = file_list[file_index];
  if (file_info.is_dir()) {
    wstring dir_path = add_trailing_slash(parent_dir) + file_info.name;
    progress.update(dir_path);

    while (true) {
      try {
        BOOL res = CreateDirectoryW(long_path(dir_path).c_str(), nullptr);
        if (!res) {
          CHECK_SYS(GetLastError() == ERROR_ALREADY_EXISTS);
        }
        break;
      }
      catch (const Error& e) {
        if (retry_or_ignore_error(dir_path, e, ignore_errors, error_log, progress))
          break;
      }
    }

    FileIndexRange dir_list = get_dir_list(file_index);
    for_each(dir_list.first, dir_list.second, [&] (UInt32 file_index) {
      prepare_extract(file_index, dir_path, indices, ignore_errors, error_log, progress);
    });
  }
  else {
    indices.push_back(file_index);
  }
}

class SetDirAttrProgress: public ProgressMonitor {
private:
  const wstring* file_path;

  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    st << Far::get_msg(MSG_PROGRESS_SET_ATTR) << L'\n';
    st << left << setw(c_width) << fit_str(*file_path, c_width) << L'\n';
    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_INDETERMINATE);

    SetConsoleTitleW(Far::get_msg(MSG_PROGRESS_SET_ATTR).c_str());
  }

public:
  SetDirAttrProgress(): ProgressMonitor(true) {
  }
  void update(const wstring& file_path) {
    this->file_path = &file_path;
    update_ui();
  }
};

void Archive::set_dir_attr(const FileIndexRange& index_range, const wstring& parent_dir, bool& ignore_errors, ErrorLog& error_log, SetDirAttrProgress& progress) {
  for_each (index_range.first, index_range.second, [&] (UInt32 file_index) {
    const FileInfo& file_info = file_list[file_index];
    wstring file_path = add_trailing_slash(parent_dir) + file_info.name;
    progress.update(file_path);
    if (file_info.is_dir()) {
      FileIndexRange dir_list = get_dir_list(file_index);
      set_dir_attr(dir_list, file_path, ignore_errors, error_log, progress);
      while (true) {
        try {
          CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), FILE_ATTRIBUTE_NORMAL));
          File file(file_path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
          CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), file_info.attr));
          file.set_time(file_info.ctime, file_info.atime, file_info.mtime);
          break;
        }
        catch (const Error& e) {
          if (retry_or_ignore_error(file_path, e, ignore_errors, error_log, progress))
            break;
        }
      }
    }
  });
}

void Archive::extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options, ErrorLog& error_log) {
  bool ignore_errors = options.ignore_errors;
  OverwriteOption overwrite_option = options.overwrite;

  prepare_dst_dir(options.dst_dir);

  PrepareExtractProgress prepare_extract_progress;
  list<UInt32> file_indices;
  for (unsigned i = 0; i < src_indices.size(); i++) {
    prepare_extract(src_indices[i], options.dst_dir, file_indices, ignore_errors, error_log, prepare_extract_progress);
  }
  prepare_extract_progress.clean();

  vector<UInt32> indices;
  indices.reserve(file_indices.size());
  copy(file_indices.begin(), file_indices.end(), back_insert_iterator<vector<UInt32>>(indices));
  sort(indices.begin(), indices.end());

  Error error;
  ExtractProgress progress;
  FileWriteCache cache(ignore_errors, error_log, progress);
  ComObject<IArchiveExtractCallback> extractor(new ArchiveExtractor(src_dir_index, options.dst_dir, file_list, password, overwrite_option, ignore_errors, error_log, error, cache, progress));
  HRESULT res = in_arc->Extract(indices.data(), static_cast<UInt32>(indices.size()), 0, extractor);
  if (FAILED(res)) {
    if (error)
      throw error;
    else
      FAIL(res);
  }
  cache.finalize();
  progress.clean();

  SetDirAttrProgress set_dir_attr_progress;
  FileIndexRange index_range(src_indices.begin(), src_indices.end());
  set_dir_attr(index_range, options.dst_dir, ignore_errors, error_log, set_dir_attr_progress);
  set_dir_attr_progress.clean();
}
