#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

/* ERROR_RETRY_BLOCK(const wstring& file_path, bool& ignore_errors, bool& error_state, ErrorLog& error_log) */

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
  const ExtractOptions& options;
  UInt64 total;
  UInt64 completed;
  wstring file_path;
  FileInfo file_info;
  OverwriteOption oo;
  bool& ignore_errors;
  ErrorLog& error_log;

  virtual void do_update_ui() {
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
    st << fit_str(file_path, 60) << L'\n';
    st << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(60, completed, total) << L'\n';
    Far::message(st.str(), 0, FMSG_LEFTALIGN);
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
      BEGIN_ERROR_RETRY_BLOCK
      h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_POSIX_SEMANTICS, NULL);
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

public:
  Error error;

  ArchiveExtractor(Archive& archive, UInt32 src_dir_index, const ExtractOptions& options, bool& ignore_errors, ErrorLog& error_log): archive(archive), src_dir_index(src_dir_index), options(options), total(0), completed(0), oo(options.overwrite), ignore_errors(ignore_errors), error_log(error_log) {
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
      file_info = archive.file_list[archive.file_id_index[index]];
      file_path = file_info.name;
      UInt32 index = file_info.parent;
      while (index != src_dir_index) {
        const FileInfo& file_info = archive.file_list[archive.file_id_index[index]];
        file_path.insert(0, 1, L'\\').insert(0, file_info.name);
        index = file_info.parent;
      }
      file_path.insert(0, 1, L'\\').insert(0, options.dst_dir);

      FindData dst_file_info;
      HANDLE h_find = FindFirstFileW(long_path(file_path).c_str(), &dst_file_info);
      if (h_find != INVALID_HANDLE_VALUE) {
        FindClose(h_find);
        bool overwrite;
        if (oo == ooAsk) {
          FindData src_file_info = convert_file_info(file_info);
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
    update_ui();
    return S_OK;
    COM_ERROR_HANDLER_END
  }
  STDMETHODIMP SetOperationResult(Int32 resultEOperationResult) {
    COM_ERROR_HANDLER_BEGIN
    bool error_state = false;
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
};


void Archive::prepare_dst_dir(const wstring& dir_path) {
  wstring path = dir_path;
  if (!is_root_path(path)) {
    prepare_dst_dir(extract_file_path(dir_path));
    CreateDirectoryW(long_path(path).c_str(), NULL);
  }
}

void Archive::prepare_extract(UInt32 dir_index, const wstring& dir_path, list<UInt32>& indices) {
  ERROR_MESSAGE_BEGIN
  BOOL res = CreateDirectoryW(long_path(dir_path).c_str(), NULL);
  if (!res) {
    CHECK_SYS(GetLastError() == ERROR_ALREADY_EXISTS);
  }
  ERROR_MESSAGE_END(dir_path)

  FileListRef fl = dir_list(dir_index);
  for (FileList::const_iterator file_info = fl.first; file_info != fl.second; file_info++) {
    if (file_info->is_dir()) {
      prepare_extract(file_info->index, add_trailing_slash(dir_path) + file_info->name, indices);
    }
    else
      indices.push_back(file_info->index);
  }
}

void Archive::set_attr(const wstring& file_path, const FileInfo& file_info, bool& ignore_errors, ErrorLog& error_log) {
  if (file_info.is_dir()) {
    FileListRef fl = dir_list(file_info.index);
    for (FileList::const_iterator file_info = fl.first; file_info != fl.second; file_info++) {
      if (file_info->is_dir()) {
        set_attr(add_trailing_slash(file_path) + file_info->name, *file_info, ignore_errors, error_log);
      }
    }
  }
  {
    bool error_state = false;
    BEGIN_ERROR_RETRY_BLOCK
    CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), FILE_ATTRIBUTE_NORMAL));
    File file(file_path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS);
    CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), file_info.attr));
    file.set_time(&file_info.ctime, &file_info.atime, &file_info.mtime);
    END_ERROR_RETRY_BLOCK
  }
}

void Archive::extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options) {
  ErrorLog error_log;
  bool ignore_errors = options.ignore_errors;
  ComObject<ArchiveExtractor> extractor(new ArchiveExtractor(*this, src_dir_index, options, ignore_errors, error_log));

  prepare_dst_dir(options.dst_dir);

  list<UInt32> file_indices;
  for (unsigned i = 0; i < src_indices.size(); i++) {
    const FileInfo& file_info = file_list[file_id_index[src_indices[i]]];
    if (file_info.is_dir()) {
      prepare_extract(file_info.index, add_trailing_slash(options.dst_dir) + file_info.name, file_indices);
    }
    else {
      file_indices.push_back(file_info.index);
    }
  }

  vector<UInt32> indices;
  indices.reserve(file_indices.size());
  copy(file_indices.begin(), file_indices.end(), back_insert_iterator<vector<UInt32>>(indices));
  sort(indices.begin(), indices.end());
  HRESULT res = in_arc->Extract(to_array(indices), indices.size(), 0, extractor);
  if (extractor->error.code != NO_ERROR)
    throw extractor->error;
  CHECK_COM(res);

  for (unsigned i = 0; i < src_indices.size(); i++) {
    const FileInfo& file_info = file_list[file_id_index[src_indices[i]]];
    set_attr(add_trailing_slash(options.dst_dir) + file_info.name, file_info, ignore_errors, error_log);
  }

  if (!error_log.empty() && options.show_dialog)
    show_error_log(error_log);
}
