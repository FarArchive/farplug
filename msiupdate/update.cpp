#include <windows.h>
#include <msi.h>

#include <process.h>
#include <time.h>

#include <string>
#include <map>
#include <sstream>
#include <vector>
using namespace std;

#include "msg.h"

#include "farutils.hpp"
#include "inet.hpp"
#include "iniparse.hpp"
#include "utils.hpp"
#include "options.hpp"
#include "update.hpp"

namespace Updater {

#ifdef _M_IX86
const char* c_upgrade_code = "{67A59013-488F-4388-81F3-828A1DCEDA32}";
const char* c_platform = "x86";
const char* c_update_script = "update2.php?p=32";
#endif
#ifdef _M_X64
const char* c_upgrade_code = "{83140F3F-1457-42EB-8053-233293664714}";
const char* c_platform = "x64";
const char* c_update_script = "update2.php?p=64";
#endif
const unsigned c_exit_wait = 6;
const unsigned c_update_period = 24 * 60 * 60;

HANDLE h_abort = NULL;
HANDLE h_update_thread = NULL;
unsigned curr_ver, update_ver;
string msi_name;

unsigned __stdcall check_thread(void* param) {
  try {
    if (check() && (update_ver > g_options.last_check_version))
      Far::call_user_apc(0);
  }
  catch (...) {
  }
  return 0;
}

void check_product_installed() {
  char product_guid[39];
  if (MsiEnumRelatedProducts(c_upgrade_code, 0, 0, product_guid) != ERROR_SUCCESS)
    FAIL_MSG(Far::get_msg(MSG_ERROR_NO_MSI));
}

void initialize() {
  g_options.load();
  h_abort = CreateEvent(NULL, TRUE, FALSE, NULL);
  CHECK_SYS(h_abort);
  check_product_installed();
  time_t curr_time = time(NULL);
  if (curr_time == -1)
    FAIL(E_FAIL);
  curr_time /= c_update_period;
  if (curr_time == g_options.last_check_time)
    return;
  curr_ver = Far::get_version();
  g_options.last_check_time = static_cast<unsigned>(curr_time);
  g_options.save();
  unsigned th_id;
  h_update_thread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, check_thread, NULL, 0, &th_id));
  CHECK_SYS(h_update_thread);
}

void finalize() {
  if (h_update_thread) {
    SetEvent(h_abort);
    DWORD wait_res = WaitForSingleObject(h_update_thread, c_exit_wait * 1000);
    if (wait_res != WAIT_OBJECT_0) {
      TerminateThread(h_update_thread, 0);
    }
    CloseHandle(h_update_thread);
  }
  if (h_abort)
    CloseHandle(h_abort);
}

string get_update_url() {
  string update_url = "http://www.farmanager.com/";
  if (g_options.update_stable_builds)
    update_url += "files/";
  else
    update_url += "nightly/";
  return update_url;
}

bool check() {
  check_product_installed();
  string update_info = load_url(widen(get_update_url() + c_update_script), h_abort);
  Ini::File update_ini;
  update_ini.parse(update_info);
  string ver_major = update_ini.get("far", "major");
  string ver_minor = update_ini.get("far", "minor");
  string ver_build = update_ini.get("far", "build");
  update_ver = MAKE_VERSION(str_to_int(ver_major), str_to_int(ver_minor), str_to_int(ver_build));
  msi_name = update_ini.get("far", "msi");
  return update_ver > curr_ver;
}

void execute() {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << Far::get_msg(MSG_UPDATE_NEW_VERSION) << L' ' << VER_MAJOR(update_ver) << L'.' << VER_MINOR(update_ver) << L'.' << VER_BUILD(update_ver) << L'\n';
  st << Far::get_msg(MSG_UPDATE_QUESTION) << L'\n';
  int res = Far::message(st.str(), 0, FMSG_MB_YESNOCANCEL);
  if (res == 0) {
    wostringstream st;
    st << L"msiexec /promptrestart ";
    if (!g_options.use_full_install_ui) {
      st << L"/qr ";
    }
    if (g_options.logged_install) {
      wchar_t temp_path[MAX_PATH];
      CHECK_SYS(GetTempPathW(ARRAYSIZE(temp_path), temp_path));
      st << L"/log \"" << add_trailing_slash(temp_path) << L"MsiUpdate_" << VER_MAJOR(curr_ver) << L"_" << widen(c_platform) << L".log\" ";
    }
    st << "/i " << widen(get_update_url() + msi_name);
    wstring command = st.str();

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    CHECK_SYS(CreateProcessW(NULL, const_cast<LPWSTR>(command.c_str()), NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi));
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  else if (res == 1) {
    g_options.last_check_version = update_ver;
    g_options.save();
  }
}

}
