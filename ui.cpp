#include <string>
#include <sstream>
#include <vector>
using namespace std;

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
  if (!e.message.empty())
    st << word_wrap(e.message, Far::get_optimal_msg_width()) << L'\n';
  st << extract_file_name(widen(e.file)) << L':' << e.line;
  Far::message(st.str(), 0, FMSG_WARNING | FMSG_MB_OK);
}

void error_dlg(const std::exception& e) {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << word_wrap(widen(e.what()), Far::get_optimal_msg_width()) << L'\n';
  Far::message(st.str(), 0, FMSG_WARNING | FMSG_MB_OK);
}

void info_dlg(const wstring& msg) {
  Far::message(Far::get_msg(MSG_PLUGIN_NAME) + L'\n' + msg, 0, FMSG_MB_OK);
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
  int use_proxy_ctrl_id;
  int proxy_server_ctrl_id;
  int proxy_port_ctrl_id;
  int proxy_auth_scheme_ctrl_id;
  int proxy_user_name_ctrl_id;
  int proxy_password_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
      options.use_full_install_ui = get_check(use_full_install_ui_ctrl_id);
      options.update_stable_builds = get_check(update_stable_builds_ctrl_id);
      options.logged_install = get_check(logged_install_ctrl_id);
      options.http.use_proxy = get_check(use_proxy_ctrl_id);
      options.http.proxy_server = get_text(proxy_server_ctrl_id);
      options.http.proxy_port = str_to_int(get_text(proxy_port_ctrl_id));
      options.http.proxy_auth_scheme = get_list_pos(proxy_auth_scheme_ctrl_id);
      options.http.proxy_user_name = get_text(proxy_user_name_ctrl_id);
      options.http.proxy_password = get_text(proxy_password_ctrl_id);
    }
    if (msg == DN_INITDIALOG) {
      bool f_enabled = get_check(use_proxy_ctrl_id);
      for (int ctrl_id = use_proxy_ctrl_id + 1; ctrl_id <= proxy_password_ctrl_id; ctrl_id++)
        enable(ctrl_id, f_enabled);
      return TRUE;
    }
    else if ((msg == DN_BTNCLICK) && (param1 == use_proxy_ctrl_id)) {
      bool f_enabled = param2 ? true : false;
      for (int ctrl_id = use_proxy_ctrl_id + 1; ctrl_id <= proxy_password_ctrl_id; ctrl_id++)
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
