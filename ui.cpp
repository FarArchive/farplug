#include "msg.h"

#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "options.hpp"
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
  unsigned __int64 t_curr;
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&t_curr));
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
  unsigned __int64 t_curr;
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

class ConfigDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 40
  };

  Options& options;

  int use_full_install_ui_ctrl_id;
  int update_stable_builds_ctrl_id;
  int logged_install_ctrl_id;
  int install_properties_ctrl_id;
  int use_proxy_ctrl_id;
  int proxy_server_ctrl_id;
  int proxy_port_ctrl_id;
  int proxy_auth_scheme_ctrl_id;
  int proxy_user_name_ctrl_id;
  int proxy_password_ctrl_id;
  int cache_enabled_ctrl_id;
  int cache_max_size_ctrl_id;
  int cache_dir_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
      options.use_full_install_ui = get_check(use_full_install_ui_ctrl_id);
      options.update_stable_builds = get_check(update_stable_builds_ctrl_id);
      options.logged_install = get_check(logged_install_ctrl_id);
      options.install_properties = get_text(install_properties_ctrl_id);
      options.http.use_proxy = get_check(use_proxy_ctrl_id);
      options.http.proxy_server = get_text(proxy_server_ctrl_id);
      options.http.proxy_port = str_to_int(get_text(proxy_port_ctrl_id));
      options.http.proxy_auth_scheme = get_list_pos(proxy_auth_scheme_ctrl_id);
      options.http.proxy_user_name = get_text(proxy_user_name_ctrl_id);
      options.http.proxy_password = get_text(proxy_password_ctrl_id);
      options.cache_enabled = get_check(cache_enabled_ctrl_id);
      options.cache_max_size = str_to_int(get_text(cache_max_size_ctrl_id));
      options.cache_dir = get_text(cache_dir_ctrl_id);
    }
    else if (msg == DN_INITDIALOG) {
      bool f_enabled = get_check(use_proxy_ctrl_id);
      for (int ctrl_id = use_proxy_ctrl_id + 1; ctrl_id <= proxy_password_ctrl_id; ctrl_id++)
        enable(ctrl_id, f_enabled);
      f_enabled = get_check(cache_enabled_ctrl_id);
      for (int ctrl_id = cache_enabled_ctrl_id + 1; ctrl_id <= cache_dir_ctrl_id; ctrl_id++)
        enable(ctrl_id, f_enabled);
      return TRUE;
    }
    else if ((msg == DN_BTNCLICK) && (param1 == use_proxy_ctrl_id)) {
      bool f_enabled = param2 ? true : false;
      for (int ctrl_id = use_proxy_ctrl_id + 1; ctrl_id <= proxy_password_ctrl_id; ctrl_id++)
        enable(ctrl_id, f_enabled);
    }
    else if ((msg == DN_BTNCLICK) && (param1 == cache_enabled_ctrl_id)) {
      bool f_enabled = param2 ? true : false;
      for (int ctrl_id = cache_enabled_ctrl_id + 1; ctrl_id <= cache_dir_ctrl_id; ctrl_id++)
        enable(ctrl_id, f_enabled);
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  ConfigDialog(Options& options): Far::Dialog(Far::get_msg(MSG_CONFIG_TITLE), c_client_xs), options(options) {
  }

  bool show() {
    use_full_install_ui_ctrl_id = check_box(Far::get_msg(MSG_CONFIG_USE_FULL_INSTALL_UI), options.use_full_install_ui);
    new_line();
    update_stable_builds_ctrl_id = check_box(Far::get_msg(MSG_CONFIG_UPDATE_STABLE_BUILDS), options.update_stable_builds);
    new_line();
    logged_install_ctrl_id = check_box(Far::get_msg(MSG_CONFIG_LOGGED_INSTALL), options.logged_install);
    new_line();
    label(Far::get_msg(MSG_CONFIG_INSTALL_PROPERTIES));
    install_properties_ctrl_id = edit_box(options.install_properties, 30);
    new_line();
    separator();
    new_line();

    use_proxy_ctrl_id = check_box(Far::get_msg(MSG_CONFIG_USE_PROXY), options.http.use_proxy);
    new_line();
    spacer(2);
    label(Far::get_msg(MSG_CONFIG_PROXY_SERVER));
    proxy_server_ctrl_id = edit_box(options.http.proxy_server, 20);
    spacer(2);
    label(Far::get_msg(MSG_CONFIG_PROXY_PORT));
    proxy_port_ctrl_id = edit_box(options.http.proxy_port ? int_to_str(options.http.proxy_port) : wstring(), 6);
    new_line();
    spacer(2);
    label(Far::get_msg(MSG_CONFIG_PROXY_AUTH_SCHEME));
    vector<wstring> auth_scheme_list;
    auth_scheme_list.push_back(Far::get_msg(MSG_CONFIG_PROXY_AUTH_BASIC));
    auth_scheme_list.push_back(Far::get_msg(MSG_CONFIG_PROXY_AUTH_NTLM));
    auth_scheme_list.push_back(Far::get_msg(MSG_CONFIG_PROXY_AUTH_PASSPORT));
    auth_scheme_list.push_back(Far::get_msg(MSG_CONFIG_PROXY_AUTH_DIGEST));
    auth_scheme_list.push_back(Far::get_msg(MSG_CONFIG_PROXY_AUTH_NEGOTIATE));
    proxy_auth_scheme_ctrl_id = combo_box(auth_scheme_list, options.http.proxy_auth_scheme, AUTO_SIZE, DIF_DROPDOWNLIST);
    new_line();
    spacer(2);
    label(Far::get_msg(MSG_CONFIG_PROXY_USER_NAME));
    proxy_user_name_ctrl_id = edit_box(options.http.proxy_user_name, 15);
    spacer(2);
    label(Far::get_msg(MSG_CONFIG_PROXY_PASSWORD));
    proxy_password_ctrl_id = edit_box(options.http.proxy_password, 15);
    new_line();
    separator();
    new_line();

    cache_enabled_ctrl_id = check_box(Far::get_msg(MSG_CONFIG_CACHE_ENABLED), options.cache_enabled);
    new_line();
    spacer(2);
    label(Far::get_msg(MSG_CONFIG_CACHE_MAX_SIZE));
    cache_max_size_ctrl_id = edit_box(int_to_str(options.cache_max_size), 2);
    new_line();
    spacer(2);
    label(Far::get_msg(MSG_CONFIG_CACHE_DIR));
    cache_dir_ctrl_id = edit_box(options.cache_dir, 30);
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

bool config_dialog(Options& options) {
  return ConfigDialog(options).show();
}
