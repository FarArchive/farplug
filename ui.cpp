#include "msg.h"

#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "ui.hpp"

ProgressMonitor::ProgressMonitor(bool lazy): h_scr(NULL) {
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
  unsigned __int64 time_curr;
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&time_curr));
  time_total += time_curr - time_cnt;
  time_cnt = time_curr;
  if ((time_total >= time_update) || force) {
    time_update = time_total + time_freq / c_update_delay_div;
    if (h_scr == NULL) {
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

ProgressSuspend::ProgressSuspend(ProgressMonitor& pm): pm(pm) {
  pm.update_ui();
}

ProgressSuspend::~ProgressSuspend() {
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&pm.time_cnt));
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
