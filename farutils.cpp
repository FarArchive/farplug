#include <string>
using namespace std;

#include "utils.hpp"
#include "farutils.hpp"

namespace Far {

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

void init(const PluginStartupInfo *psi) {
  g_far = *psi;
  g_fsf = *psi->FSF;
}

wstring get_plugin_module_path() {
  return extract_file_path(g_far.ModuleName);
}

const wchar_t* msg_ptr(int id) {
  return g_far.GetMsg(g_far.ModuleNumber, id);
}

wstring get_msg(int id) {
  return g_far.GetMsg(g_far.ModuleNumber, id);
}

unsigned get_optimal_msg_width() {
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
  if (con != INVALID_HANDLE_VALUE) {
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    if (GetConsoleScreenBufferInfo(con, &con_info)) {
      unsigned con_width = con_info.srWindow.Right - con_info.srWindow.Left + 1;
      if (con_width >= 80)
        return con_width - 20;
    }
  }
  return 60;
}

int message(const wstring& msg, int button_cnt, DWORD flags) {
  return g_far.Message(g_far.ModuleNumber, flags | FMSG_ALLINONE, NULL, reinterpret_cast<const wchar_t* const*>(msg.c_str()), 0, button_cnt);
}

};
