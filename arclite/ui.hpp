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
  bool confirm_esc;
  void update_time();
  void discard_time();
protected:
  virtual void do_update_ui() = 0;
  virtual void do_process_key(const KEY_EVENT_RECORD& ket_event) {
  }
protected:
  bool is_single_key(const KEY_EVENT_RECORD& key_event);
  void handle_esc();
public:
  ProgressMonitor(bool lazy = true);
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

enum OverwriteOption {
  ooAsk = 0,
  ooOverwrite = 1,
  ooSkip = 2,
};

struct ExtractOptions {
  wstring dst_dir;
  bool ignore_errors;
  OverwriteOption overwrite;
  bool move_enabled;
  bool move_files;
  bool show_dialog;
  wstring password;
};

bool extract_dialog(ExtractOptions& options);

enum RetryDialogResult {
  rdrRetry,
  rdrIgnore,
  rdrIgnoreAll,
  rdrCancel,
};

RetryDialogResult error_retry_ignore_dialog(const wstring& file_path, const Error& e, bool can_retry);

void show_error_log(const ErrorLog& error_log);

struct UpdateOptions {
  wstring arc_path;
  string arc_type;
  unsigned level;
  wstring method;
  bool solid;
  wstring password;
  bool show_password;
  bool encrypt;
  bool encrypt_header;
  bool encrypt_header_defined;
  bool create_sfx;
  unsigned sfx_module_idx;
  bool move_files;
  bool open_shared;
  bool ignore_errors;
};

bool update_dialog(bool new_arc, UpdateOptions& options);

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
