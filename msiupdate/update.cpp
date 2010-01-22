#include "msg.h"

#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "iniparse.hpp"
#include "utils.hpp"
#include "options.hpp"
#include "ui.hpp"
#include "inet.hpp"
#include "transact.hpp"
#include "trayicon.hpp"
#include "update.hpp"

namespace Update {

const wchar_t* c_param_last_check_time = L"last_check_time";
const wchar_t* c_param_last_check_version = L"last_check_version";
#ifdef _M_IX86
const char* c_upgrade_code = "{67A59013-488F-4388-81F3-828A1DCEDA32}";
const char* c_platform = "x86";
const wchar_t* c_update_script = L"update2.php?p=32";
#endif
#ifdef _M_X64
const char* c_upgrade_code = "{83140F3F-1457-42EB-8053-233293664714}";
const char* c_platform = "x64";
const wchar_t* c_update_script = L"update2.php?p=64";
#endif
const wchar_t* changelog_url = L"http://farmanager.com/svn/trunk/unicode_far/changelog";
const unsigned c_exit_wait = 6;
const unsigned c_update_period = 12 * 60 * 60;

unsigned current_version;

struct UpdateInfo {
  unsigned version;
  wstring package_name;
};

UpdateInfo parse_update_info(const wstring& text) {
  Ini::File update_ini;
  update_ini.parse(text);
  wstring ver_major = update_ini.get(L"far", L"major");
  wstring ver_minor = update_ini.get(L"far", L"minor");
  wstring ver_build = update_ini.get(L"far", L"build");
  UpdateInfo update_info;
  update_info.version = MAKE_VERSION(str_to_int(ver_major), str_to_int(ver_minor), str_to_int(ver_build));
  update_info.package_name = update_ini.get(L"far", L"msi");
  return update_info;
}

void check_product_installed() {
  char product_guid[39];
  if (MsiEnumRelatedProducts(c_upgrade_code, 0, 0, product_guid) != ERROR_SUCCESS)
    FAIL_MSG(Far::get_msg(MSG_ERROR_NO_MSI));
}

wstring get_update_url() {
  wstring update_url = L"http://www.farmanager.com/";
  if (g_options.update_stable_builds)
    update_url += L"files/";
  else
    update_url += L"nightly/";
  return update_url;
}

void show_changelog(unsigned build1, unsigned build2) {
  wstring text = ansi_to_unicode(load_url(changelog_url, g_options.http), 1251);
  const wchar_t* c_expr_prefix = L"[^\\s]+\\s+\\d+\\.\\d+\\.\\d+\\s+\\d+:\\d+:\\d+\\s+[+-]?\\d+\\s+-\\s+build\\s+";
  const wchar_t* c_expr_suffix = L"\\s*";
  Far::Regex regex;
  size_t pos1 = regex.search(c_expr_prefix + int_to_str(build1) + c_expr_suffix, text);
  if (pos1 == -1)
    pos1 = text.size();
  size_t pos2 = regex.search(c_expr_prefix + int_to_str(build2) + c_expr_suffix, text);
  if (pos2 == -1)
    pos2 = 0;
  CHECK(pos1 > pos2);

  TempFile temp_file;
  {
    File file(temp_file.get_path(), GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);

    const wchar_t sig = 0xFEFF;
    file.write(&sig, sizeof(sig));
    file.write(text.data() + pos2, static_cast<unsigned>((pos1 - pos2) * sizeof(wchar_t)));
  }

  Far::viewer(temp_file.get_path(), L"Changelog");
  Far::flush_screen();
}

enum UpdateDialogResult {
  udrYes,
  udrNo,
  udrCancel,
};

class UpdateDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 40
  };

  const UpdateInfo& update_info;

  int changelog_ctrl_id;
  int yes_ctrl_id;
  int no_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if (msg == DN_INITDIALOG) {
      set_focus(yes_ctrl_id);
      return TRUE;
    }
    else if ((msg == DN_BTNCLICK) && (param1 == changelog_ctrl_id)) {
      show_changelog(VER_BUILD(current_version), VER_BUILD(update_info.version));
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  UpdateDialog(const UpdateInfo& update_info): Far::Dialog(Far::get_msg(MSG_PLUGIN_NAME), c_client_xs, L"update"), update_info(update_info) {
  }

  UpdateDialogResult show() {
    wostringstream st;
    st << Far::get_msg(MSG_UPDATE_NEW_VERSION) << L' ' << VER_MAJOR(update_info.version) << L'.' << VER_MINOR(update_info.version) << L'.' << VER_BUILD(update_info.version);
    label(st.str());
    new_line();
    label(Far::get_msg(MSG_UPDATE_QUESTION));
    new_line();
    changelog_ctrl_id = button(Far::get_msg(MSG_UPDATE_CHANGELOG), DIF_BTNNOCLOSE);
    new_line();
    separator();
    new_line();
    yes_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_YES), DIF_CENTERGROUP);
    no_ctrl_id = button(Far::get_msg(MSG_BUTTON_NO), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();
    if (item == yes_ctrl_id) return udrYes;
    else if (item == no_ctrl_id) return udrNo;
    else return udrCancel;
  }
};

void prepare_directory(const wstring& dir) {
  if (GetFileAttributesW(dir.c_str()) == INVALID_FILE_ATTRIBUTES) {
    if (!is_root_path(dir)) {
      prepare_directory(extract_file_path(dir));
      CHECK_SYS(CreateDirectoryW(dir.c_str(), NULL));
    }
  }
}

void save_to_cache(const string& package, const wstring& cache_dir, const wstring& package_name) {
  prepare_directory(cache_dir);
  Transaction transaction;
  {
    const wchar_t* c_param_cache_index = L"cache_index";
    Key plugin_key(HKEY_CURRENT_USER, (add_trailing_slash(Far::get_root_key_name()) + c_plugin_key_name).c_str(), KEY_QUERY_VALUE | KEY_SET_VALUE, transaction.handle());

    list<wstring> cache_index = split(plugin_key.query_str(c_param_cache_index), L'\n');

    while (cache_index.size() && cache_index.size() >= g_options.cache_max_size) {
      delete_file_transacted((add_trailing_slash(cache_dir) + cache_index.front()).c_str(), transaction.handle());
      cache_index.erase(cache_index.begin());
    }

    File package_file((add_trailing_slash(cache_dir) + package_name).c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, transaction.handle());
    package_file.write(package.data(), static_cast<unsigned>(package.size()));

    cache_index.push_back(package_name);
    plugin_key.set_str(c_param_cache_index, combine(cache_index, L'\n'));
  }
  transaction.commit();
}

void execute(bool ask) {
  check_product_installed();
  string update_url_text = load_url(get_update_url() + c_update_script, g_options.http);
  UpdateInfo update_info = parse_update_info(ansi_to_unicode(update_url_text, CP_ACP));
  if (update_info.version <= current_version) {
    info_dlg(Far::get_msg(MSG_UPDATE_NO_NEW_VERSION));
    return;
  }
  UpdateDialogResult res = ask ? UpdateDialog(update_info).show() : udrYes;
  if (res == udrYes) {
    wstring cache_dir;
    if (g_options.cache_enabled) {
      cache_dir = expand_env_vars(g_options.cache_dir);
      string package = load_url(get_update_url() + update_info.package_name, g_options.http);
      save_to_cache(package, cache_dir, update_info.package_name);
    }

    wostringstream st;
    st << L"msiexec /promptrestart ";
    if (!g_options.use_full_install_ui) {
      st << L"/qr ";
    }
    if (g_options.logged_install) {
      wchar_t temp_path[MAX_PATH];
      CHECK_SYS(GetTempPathW(ARRAYSIZE(temp_path), temp_path));
      st << L"/log \"" << add_trailing_slash(temp_path) << L"MsiUpdate_" << VER_MAJOR(current_version) << L"_" << widen(c_platform) << L".log\" ";
    }
    if (g_options.cache_enabled)
      st << "/i \"" << add_trailing_slash(cache_dir) + update_info.package_name + L"\"";
    else
      st << "/i \"" << get_update_url() + update_info.package_name + L"\"";
    if (!g_options.use_full_install_ui) {
      st << L" LAUNCHAPP=1";
    }
    if (!g_options.install_properties.empty())
      st << L" " << g_options.install_properties;
    wstring command = st.str();

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    CHECK_SYS(CreateProcessW(NULL, const_cast<LPWSTR>(command.c_str()), NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi));
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    Far::quit();
  }
  else if (res == udrNo) {
    Options::set_int(c_param_last_check_version, update_info.version);
  }
}

class AutoUpdate: public Thread, public Event {
private:
  wstring update_url;
  HttpOptions http_options;
  wstring window_name;
  wstring msg_tray_title;
  wstring msg_tray_version;
  wstring msg_tray_update;
  virtual void run() {
    struct Clean {
      ~Clean() {
        Far::call_user_apc(reinterpret_cast<void*>(cmdClean));
      }
    };
    Clean clean;
    string update_url_text = load_url(update_url, http_options, h_event);
    UpdateInfo update_info = parse_update_info(ansi_to_unicode(update_url_text, CP_ACP));
    unsigned last_check_version = Options::get_int(c_param_last_check_version);
    if ((update_info.version > current_version) && (update_info.version > last_check_version)) {
      wostringstream st;
      st << msg_tray_version << L' ' << VER_MAJOR(update_info.version) << L'.' << VER_MINOR(update_info.version) << L'.' << VER_BUILD(update_info.version) << L'\n' << msg_tray_update;
      TrayIcon tray_icon(window_name, msg_tray_title, st.str());
      if (tray_icon.message_loop(h_event))
        Far::call_user_apc(reinterpret_cast<void*>(cmdExecute));
    }
  }
public:
  AutoUpdate(const wstring& update_url, const HttpOptions& http_options): Event(true, false), update_url(update_url), http_options(http_options), window_name(Far::get_msg(MSG_PLUGIN_NAME)), msg_tray_title(Far::get_msg(MSG_TRAY_TITLE)), msg_tray_version(Far::get_msg(MSG_TRAY_VERSION)), msg_tray_update(Far::get_msg(MSG_TRAY_UPDATE)) {
  }
  ~AutoUpdate() {
    set();
  }
};

AutoUpdate* g_auto_update = NULL;

void init() {
  current_version = Far::get_version();

  check_product_installed();

  time_t curr_time = time(NULL);
  CHECK(curr_time != -1);
  curr_time /= c_update_period;
  unsigned last_check_time = Options::get_int(c_param_last_check_time);
  if (curr_time == last_check_time)
    return;
  Options::set_int(c_param_last_check_time, static_cast<unsigned>(curr_time));

  g_auto_update = new AutoUpdate(get_update_url() + c_update_script, g_options.http);
  try {
    g_auto_update->start();
  }
  catch (...) {
    delete g_auto_update;
    g_auto_update = NULL;
    throw;
  }
}

void clean() {
  if (g_auto_update) {
    delete g_auto_update;
    g_auto_update = NULL;
  }
}

}
