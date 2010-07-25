#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

/* ERROR_RETRY_BLOCK(const wstring& file_path, bool& ignore_errors, bool& error_state, ErrorLog& error_log, ProgressMonitor& progress) */

#define BEGIN_ERROR_RETRY_BLOCK \
  if (!error_state) { \
    while (true) { \
      try {

#define END_ERROR_RETRY_BLOCK \
        break; \
      } \
      catch (const Error& e) { \
        error_state = true; \
        bool ignore = ignore_errors; \
        if (!ignore) { \
          ProgressSuspend ps(progress); \
          switch (error_retry_dialog(file_path, e)) { \
          case rdrRetry: \
            break; \
          case rdrIgnore: \
            ignore = true; \
            break; \
          case rdrIgnoreAll: \
            ignore = true; \
            ignore_errors = true; \
            break; \
          case rdrCancel: \
            FAIL(E_ABORT); \
          } \
        } \
        if (ignore) { \
          error_log.add(file_path, e); \
          break; \
        } \
      } \
    } \
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

class ArchiveExtractor: public IArchiveExtractCallback, public ICryptoGetTextPassword, public UnknownImpl, public ProgressMonitor {
private:
  Archive& archive;
  UInt32 src_dir_index;
  const vector<UInt32>& src_indices;
  const ExtractOptions& options;
  wstring file_path;
  FileInfo file_info;
  OverwriteOption oo;
  bool ignore_errors;
  Error error;
  ErrorLog& error_log;

  enum ProgressType { ptCreateDirs, ptExtract, ptSetAttr };
  ProgressType progress_type;
  UInt64 completed;
  UInt64 total;
  const wstring* progress_path;
  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
    if (progress_type == ptExtract) {
      unsigned percent;
      if (total == 0) percent = 0;
      else percent = static_cast<unsigned>(static_cast<double>(completed) * 100 / total);
      unsigned __int64 speed;
      if (time_elapsed() == 0) speed = 0;
      else speed = static_cast<unsigned>(static_cast<double>(completed) / time_elapsed() * ticks_per_sec());
      st << Far::get_msg(MSG_PROGRESS_EXTRACT) << L'\n';
      st << fit_str(file_path, c_width) << L'\n';
      st << setw(7) << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L" @ " << setw(9) << format_data_size(speed, get_speed_suffixes()) << L'\n';
      st << Far::get_progress_bar_str(c_width, percent, 100) << L'\n';
      Far::set_progress_state(TBPF_NORMAL);
      Far::set_progress_value(percent, 100);
      SetConsoleTitleW((L"{" + int_to_str(percent) + L"%} " + Far::get_msg(MSG_PROGRESS_EXTRACT)).c_str());
    }
    else if (progress_type == ptCreateDirs) {
      st << Far::get_msg(MSG_PROGRESS_CREATE_DIRS) << L'\n';
      st << left << setw(c_width) << fit_str(*progress_path, c_width) << L'\n';
      Far::set_progress_state(TBPF_INDETERMINATE);
      SetConsoleTitleW(Far::get_msg(MSG_PROGRESS_CREATE_DIRS).c_str());
    }
    else if (progress_type == ptSetAttr) {
      st << Far::get_msg(MSG_PROGRESS_SET_ATTR) << L'\n';
      st << left << setw(c_width) << fit_str(*progress_path, c_width) << L'\n';
      Far::set_progress_state(TBPF_INDETERMINATE);
      SetConsoleTitleW(Far::get_msg(MSG_PROGRESS_SET_ATTR).c_str());
    }
    Far::message(st.str(), 0, FMSG_LEFTALIGN);
  }
  void update_progress(const wstring& path) {
    progress_path = &path;
    update_ui();
  }
  void update_progress(ProgressType pt) {
    progress_type = pt;
    reset_ui();
  }

  class FileExtractStream: public ISequentialOutStream, public UnknownImpl {
  private:
    ArchiveExtractor& extractor;
    HANDLE h_file;
    const wstring& file_path;
    const FileInfo& file_info;
    bool error_state;
  public:
    FileExtractStream(ArchiveExtractor& extractor, const wstring& file_path, const FileInfo& file_info): extractor(extractor), h_file(INVALID_HANDLE_VALUE), file_path(file_path), file_info(file_info), error_state(false) {
      bool& ignore_errors = extractor.ignore_errors;
      ErrorLog& error_log = extractor.error_log;
      ProgressMonitor& progress = extractor;
      BEGIN_ERROR_RETRY_BLOCK
      h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
      CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
      END_ERROR_RETRY_BLOCK
    }
    ~FileExtractStream() {
      if (h_file != INVALID_HANDLE_VALUE)
        CloseHandle(h_file);
    }

    UNKNOWN_IMPL_BEGIN
    UNKNOWN_IMPL_ITF(ISequentialOutStream)
    UNKNOWN_IMPL_END

    STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
      Error& error = extractor.error;
      COM_ERROR_HANDLER_BEGIN
      bool& ignore_errors = extractor.ignore_errors;
      ErrorLog& error_log = extractor.error_log;
      ProgressMonitor& progress = extractor;
      BEGIN_ERROR_RETRY_BLOCK
      CHECK_SYS(WriteFile(h_file, data, size, reinterpret_cast<LPDWORD>(processedSize), NULL));
      END_ERROR_RETRY_BLOCK
      return S_OK;
      COM_ERROR_HANDLER_END;
    }
  };

  class FileSkipStream: public ISequentialOutStream, public UnknownImpl {
  public:

    UNKNOWN_IMPL_BEGIN
    UNKNOWN_IMPL_ITF(ISequentialOutStream)
    UNKNOWN_IMPL_END

    STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
      *processedSize = size;
      return S_OK;
    }
  };

  void prepare_dst_dir(const wstring& path) {
    if (!is_root_path(path)) {
      prepare_dst_dir(extract_file_path(path));
      CreateDirectoryW(long_path(path).c_str(), NULL);
    }
  }

  void prepare(UInt32 file_index, const wstring& parent_dir, list<UInt32>& indices) {
    const FileInfo& file_info = archive.file_list[file_index];
    if (file_info.is_dir()) {
      wstring dir_path = add_trailing_slash(parent_dir) + file_info.name;
      update_progress(dir_path);
      ERROR_MESSAGE_BEGIN
      BOOL res = CreateDirectoryW(long_path(dir_path).c_str(), NULL);
      if (!res) {
        CHECK_SYS(GetLastError() == ERROR_ALREADY_EXISTS);
      }
      ERROR_MESSAGE_END(file_path)

      FileIndexRange dir_list = archive.get_dir_list(file_index);
      for_each(dir_list.first, dir_list.second, [&] (UInt32 file_index) {
        prepare(file_index, dir_path, indices);
      });
    }
    else {
      indices.push_back(file_index);
    }
  }

  void set_attr(UInt32 file_index, const wstring& parent_dir) {
    const FileInfo& file_info = archive.file_list[file_index];
    wstring file_path = add_trailing_slash(parent_dir) + file_info.name;
    if (file_info.is_dir()) {
      FileIndexRange dir_list = archive.get_dir_list(file_index);
      for_each (dir_list.first, dir_list.second, [&] (UInt32 file_index) {
        if (archive.file_list[file_index].is_dir()) {
          set_attr(file_index, file_path);
        }
      });
    }
    {
      update_progress(file_path);
      bool error_state = false;
      ProgressMonitor& progress = *this;
      BEGIN_ERROR_RETRY_BLOCK
      CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), FILE_ATTRIBUTE_NORMAL));
      File file(file_path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
      CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), file_info.attr));
      file.set_time(&file_info.ctime, &file_info.atime, &file_info.mtime);
      END_ERROR_RETRY_BLOCK
    }
  }

public:

  ArchiveExtractor(Archive& archive, UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options, ErrorLog& error_log): archive(archive), src_dir_index(src_dir_index), src_indices(src_indices), options(options), total(0), completed(0), oo(options.overwrite), ignore_errors(options.ignore_errors), error_log(error_log) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IProgress)
  UNKNOWN_IMPL_ITF(IArchiveExtractCallback)
  UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
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

  STDMETHODIMP GetStream(UInt32 index, ISequentialOutStream **outStream,  Int32 askExtractMode) {
    COM_ERROR_HANDLER_BEGIN
    if (askExtractMode == NArchive::NExtract::NAskMode::kExtract) {
      file_info = archive.file_list[index];
      file_path = file_info.name;
      UInt32 parent_index = file_info.parent;
      while (parent_index != src_dir_index) {
        const FileInfo& file_info = archive.file_list[parent_index];
        file_path.insert(0, 1, L'\\').insert(0, file_info.name);
        parent_index = file_info.parent;
      }
      file_path.insert(0, 1, L'\\').insert(0, options.dst_dir);

      FindData dst_file_info;
      HANDLE h_find = FindFirstFileW(long_path(file_path).c_str(), &dst_file_info);
      if (h_find != INVALID_HANDLE_VALUE) {
        FindClose(h_find);
        bool overwrite;
        if (oo == ooAsk) {
          FindData src_file_info = convert_file_info(file_info);
          ProgressSuspend ps(*this);
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
          *outStream = NULL;
          return S_OK;
        }
      }

      ComObject<ISequentialOutStream> file_out_stream(new FileExtractStream(*this, file_path, file_info));
      file_out_stream.detach(outStream);
    }
    else {
      ComObject<ISequentialOutStream> file_skip_stream(new FileSkipStream());
      file_skip_stream.detach(outStream);
    }
    update_ui();
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
    bool error_state = false;
    ProgressMonitor& progress = *this;
    BEGIN_ERROR_RETRY_BLOCK
    if (resultEOperationResult == NArchive::NExtract::NOperationResult::kUnSupportedMethod)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_UNSUPPORTED_METHOD));
    else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kDataError)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_DATA_ERROR));
    else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kCRCError)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_CRC_ERROR));
    else
      return S_OK;
    END_ERROR_RETRY_BLOCK
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

  void extract() {
    update_progress(ptCreateDirs);
    prepare_dst_dir(options.dst_dir);

    list<UInt32> file_indices;
    for (unsigned i = 0; i < src_indices.size(); i++) {
      prepare(src_indices[i], options.dst_dir, file_indices);
    }

    update_progress(ptExtract);
    vector<UInt32> indices;
    indices.reserve(file_indices.size());
    copy(file_indices.begin(), file_indices.end(), back_insert_iterator<vector<UInt32>>(indices));
    sort(indices.begin(), indices.end());
    HRESULT res = archive.in_arc->Extract(indices.data(), indices.size(), 0, this);
    if (error.code != NO_ERROR)
      throw error;
    CHECK_COM(res);

    update_progress(ptSetAttr);
    for (unsigned i = 0; i < src_indices.size(); i++) {
      const FileInfo& file_info = archive.file_list[src_indices[i]];
      set_attr(src_indices[i], options.dst_dir);
    }
  }
};

void Archive::extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options, ErrorLog& error_log) {
  ComObject<ArchiveExtractor> extractor(new ArchiveExtractor(*this, src_dir_index, src_indices, options, error_log));
  extractor->extract();
}
