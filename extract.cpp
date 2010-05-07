#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "ui.hpp"
#include "archive.hpp"

class FileExtractStream: public ISequentialOutStream, public UnknownImpl {
private:
  HANDLE h_file;
  const wstring& file_path;
  const FileInfo& file_info;
  Error& error;
public:
  FileExtractStream(const wstring& file_path, const FileInfo& file_info, Error& error): file_path(file_path), file_info(file_info), error(error) {
    h_file = CreateFileW(long_path(file_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_POSIX_SEMANTICS, NULL);
    CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
    CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), file_info.attr));
  }
  ~FileExtractStream() {
    SetFileTime(h_file, &file_info.ctime, &file_info.atime, &file_info.mtime);
    CloseHandle(h_file);
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_END

  STDMETHODIMP Write(const void *data, UInt32 size, UInt32 *processedSize) {
    COM_ERROR_HANDLER_BEGIN
    ERROR_MESSAGE_BEGIN
    CHECK_SYS(WriteFile(h_file, data, size, reinterpret_cast<LPDWORD>(processedSize), NULL));
    return S_OK;
    ERROR_MESSAGE_END(file_path)
    COM_ERROR_HANDLER_END
  }
};


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

enum OverwriteOption { ooAsk, ooOverwrite, ooSkip };

class ArchiveExtractor: public IArchiveExtractCallback, public ICryptoGetTextPassword, public UnknownImpl, public ProgressMonitor {
private:
  Archive& archive;
  UInt32 src_dir_index;
  wstring dest_dir;
  UInt64 total;
  UInt64 completed;
  wstring file_path;
  FileInfo file_info;
  OverwriteOption oo;

  virtual void do_update_ui() {
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
    st << fit_str(file_path, 60) << L'\n';
    st << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(60, completed, total) << L'\n';
    Far::message(st.str(), 0, FMSG_LEFTALIGN);
  }
public:
  Error error;

  ArchiveExtractor(Archive& archive, UInt32 src_dir_index, const wstring& dest_dir): archive(archive), src_dir_index(src_dir_index), dest_dir(dest_dir), total(0), completed(0), oo(ooAsk) {
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
    file_info = archive.file_list[archive.file_id_index[index]];
    file_path = file_info.name;
    UInt32 index = file_info.parent;
    while (index != src_dir_index) {
      const FileInfo& file_info = archive.file_list[archive.file_id_index[index]];
      file_path.insert(0, 1, L'\\').insert(0, file_info.name);
      index = file_info.parent;
    }
    file_path.insert(0, 1, L'\\').insert(0, dest_dir);

    ERROR_MESSAGE_BEGIN
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
        CHECK_SYS(SetFileAttributesW(long_path(file_path).c_str(), FILE_ATTRIBUTE_NORMAL));
      }
      else {
        *outStream = NULL;
        return S_OK;
      }
    }

    ComObject<ISequentialOutStream> file_out_stream(new FileExtractStream(file_path, file_info, error));
    file_out_stream.detach(outStream);
    update_ui();
    return S_OK;
    ERROR_MESSAGE_END(file_path)
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
    ERROR_MESSAGE_BEGIN
    if (resultEOperationResult == NArchive::NExtract::NOperationResult::kUnSupportedMethod)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_UNSUPPORTED_METHOD));
    else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kDataError)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_DATA_ERROR));
    else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kCRCError)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_CRC_ERROR));
    else
      return S_OK;
    ERROR_MESSAGE_END(file_path)
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

void Archive::set_dir_attr(FileInfo dir_info, const wstring& dir_path) {
  FileListRef fl = dir_list(dir_info.index);
  for (FileList::const_iterator file_info = fl.first; file_info != fl.second; file_info++) {
    if (file_info->is_dir()) {
      set_dir_attr(*file_info, add_trailing_slash(dir_path) + file_info->name);
    }
  }
  {
    ERROR_MESSAGE_BEGIN
    CHECK_SYS(SetFileAttributesW(long_path(dir_path).c_str(), FILE_ATTRIBUTE_NORMAL));
    File open_dir(dir_path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS);
    CHECK_SYS(SetFileAttributesW(long_path(dir_path).c_str(), dir_info.attr));
    open_dir.set_time(&dir_info.ctime, &dir_info.atime, &dir_info.mtime);
    ERROR_MESSAGE_END(dir_path)
  }
}

void Archive::extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const wstring& dest_dir) {
  ComObject<ArchiveExtractor> extractor(new ArchiveExtractor(*this, src_dir_index, dest_dir));

  list<UInt32> file_indices;
  for (unsigned i = 0; i < src_indices.size(); i++) {
    const FileInfo& file_info = file_list[file_id_index[src_indices[i]]];
    if (file_info.is_dir()) {
      prepare_extract(file_info.index, add_trailing_slash(dest_dir) + file_info.name, file_indices);
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
  if (FAILED(res)) {
    if (extractor->error.code != NO_ERROR)
      throw extractor->error;
    CHECK_COM(res);
  }

  for (unsigned i = 0; i < src_indices.size(); i++) {
    const FileInfo& file_info = file_list[file_id_index[src_indices[i]]];
    if (file_info.is_dir()) {
      set_dir_attr(file_info, add_trailing_slash(dest_dir) + file_info.name);
    }
  }
}
