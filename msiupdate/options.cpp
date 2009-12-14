#include <windows.h>

#include <string>
#include <vector>
using namespace std;

#include "utils.hpp"
#include "farutils.hpp"
#include "options.hpp"

Options g_options;

const wchar_t* c_plugin_key_name = L"msiupdate";

const wchar_t* c_param_last_check_time = L"last_check_time";
const wchar_t* c_param_last_check_version = L"last_check_version";
const wchar_t* c_param_use_full_install_ui = L"use_full_install_ui";
const wchar_t* c_param_update_stable_builds = L"update_stable_builds";
const wchar_t* c_param_logged_install = L"logged_install";
const wchar_t* c_param_use_proxy = L"use_proxy";
const wchar_t* c_param_proxy_server = L"proxy_server";
const wchar_t* c_param_proxy_port = L"proxy_port";
const wchar_t* c_param_proxy_auth_scheme = L"proxy_auth_scheme";
const wchar_t* c_param_proxy_user_name = L"proxy_user_name";
const wchar_t* c_param_proxy_password = L"proxy_password";

unsigned Options::get_int(const wchar_t* name, unsigned def_value) {
  unsigned value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, Far::get_root_key_name().c_str(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_DWORD;
      DWORD data;
      DWORD data_size = sizeof(data);
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, reinterpret_cast<LPBYTE>(&data), &data_size);
      if (res == ERROR_SUCCESS) value = data;
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

bool Options::get_bool(const wchar_t* name, bool def_value) {
  bool value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, Far::get_root_key_name().c_str(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_DWORD;
      DWORD data;
      DWORD data_size = sizeof(data);
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, reinterpret_cast<LPBYTE>(&data), &data_size);
      if (res == ERROR_SUCCESS) value = data != 0;
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

wstring Options::get_str(const wchar_t* name, const wstring& def_value) {
  wstring value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, Far::get_root_key_name().c_str(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_SZ;
      DWORD data_size;
      // get string size
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, NULL, &data_size);
      if (res == ERROR_SUCCESS) {
        Buffer<wchar_t> buf(data_size / sizeof(wchar_t));
        // get string value
        res = RegQueryValueExW(h_plugin_key, name, NULL, &type, reinterpret_cast<LPBYTE>(buf.data()), &data_size);
        if (res == ERROR_SUCCESS) {
          value.assign(buf.data(), buf.size() - 1); // throw away terminating NULL
        }
      }
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

void Options::set_int(const wchar_t* name, unsigned value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, Far::get_root_key_name().c_str(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      DWORD data = value;
      RegSetValueExW(h_plugin_key, name, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&data), sizeof(data));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void Options::set_bool(const wchar_t* name, bool value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, Far::get_root_key_name().c_str(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      DWORD data = value ? 1 : 0;
      RegSetValueExW(h_plugin_key, name, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&data), sizeof(data));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void Options::set_str(const wchar_t* name, const wstring& value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, Far::get_root_key_name().c_str(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      // size should include terminating NULL
      RegSetValueExW(h_plugin_key, name, 0, REG_SZ, reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(value.c_str())), (static_cast<DWORD>(value.size()) + 1) * sizeof(wchar_t));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void Options::load() {
  last_check_time = get_int(c_param_last_check_time);
  last_check_version = get_int(c_param_last_check_version);
  use_full_install_ui = get_bool(c_param_use_full_install_ui);
  update_stable_builds = get_bool(c_param_update_stable_builds);
  logged_install = get_bool(c_param_logged_install, true);
  http.use_proxy = get_bool(c_param_use_proxy);
  http.proxy_server = get_str(c_param_proxy_server);
  http.proxy_port = get_int(c_param_proxy_port);
  http.proxy_auth_scheme = get_int(c_param_proxy_auth_scheme);
  http.proxy_user_name = get_str(c_param_proxy_user_name);
  http.proxy_password = get_str(c_param_proxy_password);

};

void Options::save() {
  set_int(c_param_last_check_time, last_check_time);
  set_int(c_param_last_check_version, last_check_version);
  set_bool(c_param_use_full_install_ui, use_full_install_ui);
  set_bool(c_param_update_stable_builds, update_stable_builds);
  set_bool(c_param_logged_install, logged_install);
  set_bool(c_param_use_proxy, http.use_proxy);
  set_str(c_param_proxy_server, http.proxy_server);
  set_int(c_param_proxy_port, http.proxy_port);
  set_int(c_param_proxy_auth_scheme, http.proxy_auth_scheme);
  set_str(c_param_proxy_user_name, http.proxy_user_name);
  set_str(c_param_proxy_password, http.proxy_password);
}
