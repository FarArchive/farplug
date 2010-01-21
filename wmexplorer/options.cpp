#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "util.h"
#include "options.h"

extern struct PluginStartupInfo g_far;
extern struct FarStandardFunctions g_fsf;

const wchar_t* c_plugin_key_name = L"WMExplorer";

UnicodeString get_root_key_name() {
  return FARSTR_TO_UNICODE(g_far.RootKey);
}

int get_int_option(const wchar_t* name, int def_value) {
  int value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_DWORD;
      DWORD data;
      DWORD data_size = sizeof(data);
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, (LPBYTE) &data, &data_size);
      if (res == ERROR_SUCCESS) value = data;
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

bool get_bool_option(const wchar_t* name, bool def_value) {
  bool value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_DWORD;
      DWORD data;
      DWORD data_size = sizeof(data);
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, (LPBYTE) &data, &data_size);
      if (res == ERROR_SUCCESS) value = data != 0;
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

UnicodeString get_str_option(const wchar_t* name, const UnicodeString& def_value) {
  UnicodeString value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_SZ;
      DWORD data_size;
      // get string size
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, NULL, &data_size);
      if (res == ERROR_SUCCESS) {
        UnicodeString data;
        // get string value
        res = RegQueryValueExW(h_plugin_key, name, NULL, &type, (LPBYTE) data.buf(data_size / sizeof(wchar_t)), &data_size);
        if (res == ERROR_SUCCESS) {
          data.set_size(data_size / sizeof(wchar_t) - 1); // throw away terminating NULL
          value = data;
        }
      }
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

bool virt_key_from_name(const UnicodeString& name, unsigned& key, unsigned& state) {
  state = 0;
  unsigned far_key = g_fsf.FarNameToKey(UNICODE_TO_FARSTR(name).data());
  if (far_key == -1) return false;
  if ((far_key & (KEY_CTRL | KEY_RCTRL)) != 0) state |= PKF_CONTROL;
  if ((far_key & (KEY_ALT | KEY_RALT)) != 0) state |= PKF_ALT;
  if ((far_key & KEY_SHIFT) != 0) state |= PKF_SHIFT;
  key = far_key & ~KEY_CTRLMASK;
  if (key > KEY_FKEY_BEGIN) {
    key -= KEY_FKEY_BEGIN;
    if (key > 0xFF) return false;
  }
  return true;
}

KeyOption get_key_option(const wchar_t* name, const KeyOption& def_key) {
  KeyOption value;
  UnicodeString key_name = get_str_option(name, UnicodeString());
  if ((key_name.size() == 0) || (!virt_key_from_name(key_name, value.vcode, value.state))) {
    value = def_key;
  }
  return value;
}

void set_int_option(const wchar_t* name, int value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      DWORD data = value;
      RegSetValueExW(h_plugin_key, name, 0, REG_DWORD, (LPBYTE) &data, sizeof(data));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void set_bool_option(const wchar_t* name, bool value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      DWORD data = value ? 1 : 0;
      RegSetValueExW(h_plugin_key, name, 0, REG_DWORD, (LPBYTE) &data, sizeof(data));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void set_str_option(const wchar_t* name, const UnicodeString& value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      // size should include terminating NULL
      RegSetValueExW(h_plugin_key, name, 0, REG_SZ, (LPBYTE) value.data(), (value.size() + 1) * sizeof(wchar_t));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void load_plugin_options(PluginOptions& options) {
  options.add_to_plugin_menu = get_bool_option(L"add_to_plugin_menu", true);
  options.add_to_disk_menu = get_bool_option(L"add_to_disk_menu", true);
  options.disk_menu_number = get_int_option(L"disk_menu_number", -1);
  options.access_method = (AccessMethod) get_int_option(L"access_method", amRapi2);
  options.hide_copy_dlg = get_bool_option(L"hide_copy_dlg", false);
  options.ignore_errors = get_bool_option(L"ignore_errors", false);
  options.overwrite = (OverwriteOption) get_int_option(L"overwrite", ooAsk);
  options.show_stats = (ShowStatsOption) get_int_option(L"show_op_stats", ssoIfError);
  options.last_dev_type = (DevType) get_int_option(L"last_dev_type", dtPDA);
  options.copy_buf_size = get_int_option(L"copy_buf_size", -1);
  options.hide_rom_files = get_bool_option(L"hide_rom_files", false);
  options.save_def_values = get_bool_option(L"save_def_values", true);
  options.key_attr = get_key_option(L"key_attr", KeyOption('A', PKF_CONTROL));
  options.key_execute = get_key_option(L"key_execute", KeyOption('S', PKF_CONTROL | PKF_ALT));
  options.key_hide_rom_files = get_key_option(L"key_hide_rom_files", KeyOption('I', PKF_CONTROL | PKF_ALT));
  options.save_last_dir = get_bool_option(L"save_last_dir", false);
  options.use_file_filters = get_bool_option(L"use_file_filters", false);
  options.exit_on_dot_dot = get_bool_option(L"exit_on_dot_dot", true);
  options.prefix = get_str_option(L"prefix", L"WMExplorer");
  options.show_free_space = get_bool_option(L"show_free_space", true);
}

void save_plugin_options(const PluginOptions& options) {
  set_bool_option(L"add_to_plugin_menu", options.add_to_plugin_menu);
  set_bool_option(L"add_to_disk_menu", options.add_to_disk_menu);
  set_int_option(L"disk_menu_number", options.disk_menu_number);
  set_int_option(L"access_method", options.access_method);
  set_int_option(L"hide_copy_dlg", options.hide_copy_dlg);
  set_int_option(L"copy_buf_size", options.copy_buf_size);
  set_bool_option(L"hide_rom_files", options.hide_rom_files);
  set_bool_option(L"ignore_errors", options.ignore_errors);
  set_int_option(L"overwrite", options.overwrite);
  set_int_option(L"show_op_stats", options.show_stats);
  set_bool_option(L"use_file_filters", options.use_file_filters);
  set_bool_option(L"save_def_values", options.save_def_values);
  set_int_option(L"last_dev_type", options.last_dev_type);
  set_bool_option(L"save_last_dir", options.save_last_dir);
  set_bool_option(L"exit_on_dot_dot", options.exit_on_dot_dot);
  set_str_option(L"prefix", options.prefix);
  set_bool_option(L"show_free_space", options.show_free_space);
}

void save_def_option_values(const PluginOptions& options) {
  set_bool_option(L"ignore_errors", options.ignore_errors);
  set_int_option(L"overwrite", options.overwrite);
  set_int_option(L"show_op_stats", options.show_stats);
  set_bool_option(L"use_file_filters", options.use_file_filters);
}
