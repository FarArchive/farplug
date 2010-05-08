#pragma once

class ProgressMonitor: private NonCopyable {
private:
  HANDLE h_scr;
  wstring con_title;
  static const unsigned c_first_delay_div = 2;
  static const unsigned c_update_delay_div = 2;
  unsigned __int64 time_cnt;
  unsigned __int64 time_freq;
  unsigned __int64 time_total;
  unsigned __int64 time_update;
protected:
  virtual void do_update_ui() = 0;
public:
  ProgressMonitor(bool lazy = true);
  virtual ~ProgressMonitor();
  void update_ui(bool force = false);
  void update_time();
  void discard_time();
  unsigned __int64 time_elapsed();
  unsigned __int64 ticks_per_sec();
  friend class ProgressSuspend;
};

class ProgressSuspend: private NonCopyable {
private:
  ProgressMonitor& pm;
public:
  ProgressSuspend::ProgressSuspend(ProgressMonitor& pm): pm(pm) {
    pm.update_time();
  }
  ProgressSuspend::~ProgressSuspend() {
    pm.discard_time();
  }
};

const wchar_t** get_size_suffixes();
const wchar_t** get_speed_suffixes();

bool password_dialog(wstring& password);

enum OverwriteAction { oaYes, oaYesAll, oaNo, oaNoAll, oaCancel };
OverwriteAction overwrite_dialog(const wstring& file_path, const FindData& src_file_info, const FindData& dst_file_info);

enum OverwriteOption {
  ooAsk = 0,
  ooOverwrite = 1,
  ooSkip = 2,
};

struct ExtractOptions {
  wstring dst_dir;
  bool ignore_errors;
  OverwriteOption overwrite;
  bool move_files;
  bool show_dialog;
  bool use_tmp_files;
};

bool extract_dialog(ExtractOptions& options);
