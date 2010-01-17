#include "msg.h"

#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "ui.hpp"

void error_dlg(const Error& e) {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  if (e.code != NO_ERROR) {
    wstring sys_msg = get_system_message(e.code);
    if (!sys_msg.empty())
      st << word_wrap(sys_msg, Far::get_optimal_msg_width()) << L'\n';
  }
  for (list<wstring>::const_iterator message = e.messages.begin(); message != e.messages.end(); message++) {
    st << word_wrap(*message, Far::get_optimal_msg_width()) << L'\n';
  }
  st << extract_file_name(widen(e.file)) << L':' << e.line;
  Far::message(st.str(), 0, FMSG_WARNING | FMSG_MB_OK);
}

void error_dlg(const std::exception& e) {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << word_wrap(widen(e.what()), Far::get_optimal_msg_width()) << L'\n';
  Far::message(st.str(), 0, FMSG_WARNING | FMSG_MB_OK);
}

void error_dlg(const wstring& msg) {
  Far::message(Far::get_msg(MSG_PLUGIN_NAME) + L'\n' + msg, 0, FMSG_WARNING | FMSG_MB_OK);
}

void info_dlg(const wstring& msg) {
  Far::message(Far::get_msg(MSG_PLUGIN_NAME) + L'\n' + msg, 0, FMSG_MB_OK);
}

ProgressMonitor::ProgressMonitor(bool lazy): h_scr(NULL) {
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&t_curr));
  t_start = t_curr;
  QueryPerformanceFrequency(reinterpret_cast<PLARGE_INTEGER>(&t_freq));
  if (lazy)
    t_next = t_curr + t_freq / 2;
  else
    t_next = t_curr;
}

ProgressMonitor::~ProgressMonitor() {
  if (h_scr) {
    Far::restore_screen(h_scr);
    SetConsoleTitleW(con_title.data());
    Far::set_progress_state(TBPF_NOPROGRESS);
  }
}

void ProgressMonitor::update_ui(bool force) {
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&t_curr));
  if ((t_curr >= t_next) || force) {
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
    t_next = t_curr + t_freq / 2;
    do_update_ui();
  }
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
