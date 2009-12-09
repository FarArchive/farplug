#include <string>
#include <sstream>
using namespace std;

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
