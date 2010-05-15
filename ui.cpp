#include "msg.h"

#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"

wstring get_error_dlg_title() {
  return Far::get_msg(MSG_PLUGIN_NAME);
}

ProgressMonitor::ProgressMonitor(bool lazy): h_scr(nullptr) {
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&time_cnt));
  QueryPerformanceFrequency(reinterpret_cast<PLARGE_INTEGER>(&time_freq));
  time_total = 0;
  if (lazy)
    time_update = time_total + time_freq / c_first_delay_div;
  else
    time_update = time_total;
}

ProgressMonitor::~ProgressMonitor() {
  if (h_scr) {
    Far::restore_screen(h_scr);
    Far::flush_screen();
    SetConsoleTitleW(con_title.data());
    Far::set_progress_state(TBPF_NOPROGRESS);
  }
}

void ProgressMonitor::update_ui(bool force) {
  update_time();
  if ((time_total >= time_update) || force) {
    time_update = time_total + time_freq / c_update_delay_div;
    if (h_scr == nullptr) {
      Far::flush_screen();
      h_scr = Far::save_screen();
      con_title = get_console_title();
    }
    HANDLE h_con = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD rec;
    DWORD read_cnt;
    while (true) {
      PeekConsoleInput(h_con, &rec, 1, &read_cnt);
      if (read_cnt == 0) break;
      ReadConsoleInput(h_con, &rec, 1, &read_cnt);
      if ((rec.EventType == KEY_EVENT) && (rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) && rec.Event.KeyEvent.bKeyDown && ((rec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED | SHIFT_PRESSED)) == 0)) FAIL(E_ABORT);
    }
    do_update_ui();
  }
}

void ProgressMonitor::reset_ui() {
  time_total = time_update = 0;
  if (h_scr) {
    Far::restore_screen(h_scr);
    h_scr = nullptr;
  }
}

void ProgressMonitor::update_time() {
  unsigned __int64 time_curr;
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&time_curr));
  time_total += time_curr - time_cnt;
  time_cnt = time_curr;
}

void ProgressMonitor::discard_time() {
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&time_cnt));
}

unsigned __int64 ProgressMonitor::time_elapsed() {
  update_time();
  return time_total;
}

unsigned __int64 ProgressMonitor::ticks_per_sec() {
  return time_freq;
}

const wchar_t** get_size_suffixes() {
  static const wchar_t* suffixes[5] = {
    L"",
    Far::msg_ptr(MSG_SUFFIX_SIZE_KB),
    Far::msg_ptr(MSG_SUFFIX_SIZE_MB),
    Far::msg_ptr(MSG_SUFFIX_SIZE_GB),
    Far::msg_ptr(MSG_SUFFIX_SIZE_TB),
  };
  return suffixes;
}

const wchar_t** get_speed_suffixes() {
  static const wchar_t* suffixes[5] = {
    Far::msg_ptr(MSG_SUFFIX_SPEED_B),
    Far::msg_ptr(MSG_SUFFIX_SPEED_KB),
    Far::msg_ptr(MSG_SUFFIX_SPEED_MB),
    Far::msg_ptr(MSG_SUFFIX_SPEED_GB),
    Far::msg_ptr(MSG_SUFFIX_SPEED_TB),
  };
  return suffixes;
}

class PasswordDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 40
  };

  wstring& password;

  int password_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
      password = get_text(password_ctrl_id);
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  PasswordDialog(wstring& password): Far::Dialog(Far::get_msg(MSG_PASSWORD_TITLE), c_client_xs), password(password) {
  }

  bool show() {
    label(Far::get_msg(MSG_PASSWORD_PASSWORD));
    password_ctrl_id = pwd_edit_box(password);
    new_line();
    separator();
    new_line();

    ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    return (item != -1) && (item != cancel_ctrl_id);
  }
};

bool password_dialog(wstring& password) {
  return PasswordDialog(password).show();
}

class OverwriteDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 60
  };

  int yes_ctrl_id;
  int yes_all_ctrl_id;
  int no_ctrl_id;
  int no_all_ctrl_id;
  int cancel_ctrl_id;

public:
  OverwriteDialog(): Far::Dialog(Far::get_msg(MSG_OVERWRITE_DLG_TITLE), c_client_xs) {
  }

  OverwriteAction show(const wstring& file_path, const FindData& src_file_info, const FindData& dst_file_info) {
    label(fit_str(file_path, c_client_xs));
    new_line();
    label(Far::get_msg(MSG_OVERWRITE_DLG_QUESTION));
    new_line();
    separator();
    new_line();

    label(Far::get_msg(MSG_OVERWRITE_DLG_SOURCE));
    pad(15);
    if (!src_file_info.is_dir()) {
      label(format_data_size(src_file_info.size(), get_size_suffixes()));
      pad(25);
    }
    label(format_file_time(src_file_info.ftLastWriteTime));
    if (CompareFileTime(&src_file_info.ftLastWriteTime, &dst_file_info.ftLastWriteTime) > 0) {
      spacer(1);
      label(Far::get_msg(MSG_OVERWRITE_DLG_NEWER));
    }
    new_line();

    label(Far::get_msg(MSG_OVERWRITE_DLG_DESTINATION));
    pad(15);
    if (!dst_file_info.is_dir()) {
      label(format_data_size(dst_file_info.size(), get_size_suffixes()));
      pad(25);
    }
    label(format_file_time(dst_file_info.ftLastWriteTime));
    if (CompareFileTime(&src_file_info.ftLastWriteTime, &dst_file_info.ftLastWriteTime) < 0) {
      spacer(1);
      label(Far::get_msg(MSG_OVERWRITE_DLG_NEWER));
    }
    new_line();

    separator();
    new_line();
    yes_ctrl_id = def_button(Far::get_msg(MSG_OVERWRITE_DLG_YES), DIF_CENTERGROUP);
    yes_all_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_YES_ALL), DIF_CENTERGROUP);
    no_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_NO), DIF_CENTERGROUP);
    no_all_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_NO_ALL), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    if (item == yes_ctrl_id) return oaYes;
    else if (item == yes_all_ctrl_id) return oaYesAll;
    else if (item == no_ctrl_id) return oaNo;
    else if (item == no_all_ctrl_id) return oaNoAll;
    else return oaCancel;
  }
};

OverwriteAction overwrite_dialog(const wstring& file_path, const FindData& src_file_info, const FindData& dst_file_info) {
  return OverwriteDialog().show(file_path, src_file_info, dst_file_info);
}

class ExtractDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 60
  };

  ExtractOptions& options;

  int dst_dir_ctrl_id;
  int ignore_errors_ctrl_id;
  int oo_ask_ctrl_id;
  int oo_overwrite_ctrl_id;
  int oo_skip_ctrl_id;
  int move_files_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
      options.dst_dir = get_text(dst_dir_ctrl_id);
      options.dst_dir = unquote(strip(options.dst_dir));
      options.ignore_errors = get_check(ignore_errors_ctrl_id);
      if (get_check(oo_ask_ctrl_id)) options.overwrite = ooAsk;
      else if (get_check(oo_overwrite_ctrl_id)) options.overwrite = ooOverwrite;
      else options.overwrite = ooSkip;
      options.move_files = get_check(move_files_ctrl_id);
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  ExtractDialog(ExtractOptions& options): Far::Dialog(Far::get_msg(MSG_EXTRACT_DLG_TITLE), c_client_xs), options(options) {
  }

  bool show() {
    label(Far::get_msg(MSG_EXTRACT_DLG_DST_DIR));
    new_line();
    dst_dir_ctrl_id = edit_box(options.dst_dir, c_client_xs);
    new_line();
    separator();
    new_line();

    ignore_errors_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_IGNORE_ERRORS), options.ignore_errors);
    new_line();

    label(Far::get_msg(MSG_EXTRACT_DLG_OO));
    new_line();
    spacer(2);
    oo_ask_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OO_ASK), options.overwrite == ooAsk);
    spacer(2);
    oo_overwrite_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OO_OVERWRITE), options.overwrite == ooOverwrite);
    spacer(2);
    oo_skip_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OO_SKIP), options.overwrite == ooSkip);
    new_line();

    move_files_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_MOVE_FILES), options.move_files, options.move_enabled ? 0 : DIF_DISABLE);
    new_line();

    separator();
    new_line();
    ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    return (item != -1) && (item != cancel_ctrl_id);
  }
};

bool extract_dialog(ExtractOptions& options) {
  return ExtractDialog(options).show();
}

RetryDialogResult error_retry_dialog(const wstring& file_path, const Error& e) {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << fit_str(file_path, Far::get_optimal_msg_width()) << L'\n';
  if (e.code != E_MESSAGE) {
    wstring sys_msg = get_system_message(e.code);
    if (!sys_msg.empty())
      st << word_wrap(sys_msg, Far::get_optimal_msg_width()) << L'\n';
  }
  for (list<wstring>::const_iterator msg = e.messages.begin(); msg != e.messages.end(); msg++) {
    st << word_wrap(*msg, Far::get_optimal_msg_width()) << L'\n';
  }
  st << extract_file_name(widen(e.file)) << L':' << e.line << L'\n';
  st << Far::get_msg(MSG_BUTTON_RETRY) << L'\n';
  st << Far::get_msg(MSG_BUTTON_IGNORE) << L'\n';
  st << Far::get_msg(MSG_BUTTON_IGNORE_ALL) << L'\n';
  st << Far::get_msg(MSG_BUTTON_CANCEL) << L'\n';
  switch (Far::message(st.str(), 4, FMSG_WARNING)) {
  case 0:
    return rdrRetry;
  case 1:
    return rdrIgnore;
  case 2:
    return rdrIgnoreAll;
  default:
    return rdrCancel;
  }
}

void show_error_log(const ErrorLog& error_log) {
  wstring msg;
  msg += Far::get_msg(MSG_LOG_TITLE) + L'\n';
  msg += Far::get_msg(MSG_LOG_INFO) + L'\n';
  msg += Far::get_msg(MSG_LOG_CLOSE) + L'\n';
  msg += Far::get_msg(MSG_LOG_SHOW) + L'\n';
  if (Far::message(msg, 2, FMSG_WARNING) != 1) return;

  TempFile temp_file;
  File file(temp_file.get_path(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);

  const wchar_t sig = 0xFEFF;
  file.write(&sig, sizeof(sig));
  wstring line;
  for (ErrorLog::const_iterator iter = error_log.begin(); iter != error_log.end(); iter++) {
    const wstring& file_path = iter->first;
    const Error& error = iter->second;
    line.assign(file_path).append(1, L'\n');
    if (error.code != E_MESSAGE) {
      wstring sys_msg = get_system_message(error.code);
      if (!sys_msg.empty())
        line.append(word_wrap(sys_msg, Far::get_optimal_msg_width())).append(1, L'\n');
    }
    for (list<wstring>::const_iterator err_msg = error.messages.begin(); err_msg != error.messages.end(); err_msg++) {
      line.append(word_wrap(*err_msg, Far::get_optimal_msg_width())).append(1, L'\n');
    }
    line.append(1, L'\n');
    file.write(line.data(), line.size() * sizeof(wchar_t));
  }

  Far::viewer(temp_file.get_path(), Far::get_msg(MSG_LOG_TITLE));
}
