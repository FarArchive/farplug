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
  bool paused;
  bool low_priority;
  DWORD initial_priority;
  bool confirm_esc;
  void update_time();
  void discard_time();
  void display();
protected:
  wstring progress_title;
  wstring progress_text;
  bool progress_known;
  unsigned percent_done;
  virtual void do_update_ui() = 0;
protected:
  bool is_single_key(const KEY_EVENT_RECORD& key_event);
  void handle_esc();
public:
  ProgressMonitor(const wstring& progress_title, bool progress_known = true, bool lazy = true);
  virtual ~ProgressMonitor();
  void update_ui(bool force = false);
  void clean();
  unsigned __int64 time_elapsed();
  unsigned __int64 ticks_per_sec();
  friend class ProgressSuspend;
};

class ProgressSuspend: private NonCopyable {
private:
  ProgressMonitor& progress;
public:
  ProgressSuspend::ProgressSuspend(ProgressMonitor& progress): progress(progress) {
    progress.update_time();
  }
  ProgressSuspend::~ProgressSuspend() {
    progress.discard_time();
  }
};

const wchar_t** get_size_suffixes();
const wchar_t** get_speed_suffixes();

bool password_dialog(wstring& password);

enum OverwriteAction { oaYes, oaYesAll, oaNo, oaNoAll, oaCancel };
OverwriteAction overwrite_dialog(const wstring& file_path, const FindData& src_file_info, const FindData& dst_file_info);

bool extract_dialog(ExtractOptions& options);

enum RetryDialogResult {
  rdrRetry,
  rdrIgnore,
  rdrIgnoreAll,
  rdrCancel,
};

RetryDialogResult error_retry_ignore_dialog(const wstring& file_path, const Error& e, bool can_retry);

void show_error_log(const ErrorLog& error_log);

bool update_dialog(bool new_arc, UpdateOptions& options, UpdateProfiles& profiles);

struct PluginSettings {
  bool handle_create;
  bool handle_commands;
  bool use_include_masks;
  wstring include_masks;
  bool use_exclude_masks;
  wstring exclude_masks;
};

bool settings_dialog(PluginSettings& settings);

void attr_dialog(const AttrList& attr_list);

bool sfx_convert_dialog(wstring& sfx_module_idx);
