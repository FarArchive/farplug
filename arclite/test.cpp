#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"

class ArchiveTester: public IArchiveExtractCallback, public ICryptoGetTextPassword, public ComBase, public ProgressMonitor {
private:
  UInt32 src_dir_index;
  const FileList& file_list;
  wstring password;

  wstring file_path;
  unsigned __int64 completed;
  unsigned __int64 total;
  virtual void do_update_ui() {
    const unsigned c_width = 60;
    wostringstream st;
    st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';

    unsigned percent = calc_percent(completed, total);

    unsigned __int64 speed;
    if (time_elapsed() == 0)
      speed = 0;
    else
      speed = round(static_cast<double>(completed) / time_elapsed() * ticks_per_sec());

    st << Far::get_msg(MSG_PROGRESS_TEST) << L'\n';
    st << fit_str(file_path, c_width) << L'\n';
    st << setw(7) << format_data_size(completed, get_size_suffixes()) << L" / " << format_data_size(total, get_size_suffixes()) << L" @ " << setw(9) << format_data_size(speed, get_speed_suffixes()) << L'\n';
    st << Far::get_progress_bar_str(c_width, percent, 100) << L'\n';

    Far::message(st.str(), 0, FMSG_LEFTALIGN);

    Far::set_progress_state(TBPF_NORMAL);
    Far::set_progress_value(percent, 100);

    SetConsoleTitleW((L"{" + int_to_str(percent) + L"%} " + Far::get_msg(MSG_PROGRESS_TEST)).c_str());
  }

public:
  ArchiveTester(UInt32 src_dir_index, const FileList& file_list, Error& error): ComBase(error), ProgressMonitor(true), src_dir_index(src_dir_index), file_list(file_list), completed(0), total(0) {
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
    if (completeValue) {
      completed = *completeValue;
      update_ui();
    }
    return S_OK;
    COM_ERROR_HANDLER_END
  }

  STDMETHODIMP GetStream(UInt32 index, ISequentialOutStream **outStream,  Int32 askExtractMode) {
    COM_ERROR_HANDLER_BEGIN
    const FileInfo& file_info = file_list[index];
    file_path = file_info.name;
    UInt32 parent_index = file_info.parent;
    while (parent_index != src_dir_index) {
      const FileInfo& file_info = file_list[parent_index];
      file_path.insert(0, 1, L'\\').insert(0, file_info.name);
      parent_index = file_info.parent;
    }
    update_ui();
    *outStream = nullptr;
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
    if (resultEOperationResult == NArchive::NExtract::NOperationResult::kUnSupportedMethod)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_UNSUPPORTED_METHOD));
    else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kDataError)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_DATA_ERROR));
    else if (resultEOperationResult == NArchive::NExtract::NOperationResult::kCRCError)
      FAIL_MSG(Far::get_msg(MSG_ERROR_EXTRACT_CRC_ERROR));
    else
      return S_OK;
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

void Archive::prepare_test(UInt32 file_index, list<UInt32>& indices) {
  const FileInfo& file_info = file_list[file_index];
  if (file_info.is_dir()) {
    FileIndexRange dir_list = get_dir_list(file_index);
    for_each(dir_list.first, dir_list.second, [&] (UInt32 file_index) {
      prepare_test(file_index, indices);
    });
  }
  else {
    indices.push_back(file_index);
  }
}

void Archive::test(UInt32 src_dir_index, const vector<UInt32>& src_indices) {
  list<UInt32> file_indices;
  for (unsigned i = 0; i < src_indices.size(); i++) {
    prepare_test(src_indices[i], file_indices);
  }

  vector<UInt32> indices;
  indices.reserve(file_indices.size());
  copy(file_indices.begin(), file_indices.end(), back_insert_iterator<vector<UInt32>>(indices));
  sort(indices.begin(), indices.end());

  Error error;
  ComObject<IArchiveExtractCallback> tester(new ArchiveTester(src_dir_index, file_list, error));
  HRESULT res = in_arc->Extract(indices.data(), static_cast<UInt32>(indices.size()), 1, tester);
  if (FAILED(res)) {
    if (error)
      throw error;
    else
      FAIL(res);
  }
}
