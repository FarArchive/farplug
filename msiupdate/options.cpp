#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "options.hpp"

Options g_options;

const wchar_t* c_plugin_key_name = L"msiupdate";

const wchar_t* c_param_use_full_install_ui = L"use_full_install_ui";
const wchar_t* c_param_update_stable_builds = L"update_stable_builds";
const wchar_t* c_param_logged_install = L"logged_install";
const wchar_t* c_param_install_properties = L"install_properties";
const wchar_t* c_param_use_proxy = L"use_proxy";
const wchar_t* c_param_proxy_server = L"proxy_server";
const wchar_t* c_param_proxy_port = L"proxy_port";
const wchar_t* c_param_proxy_auth_scheme = L"proxy_auth_scheme";
const wchar_t* c_param_proxy_user_name = L"proxy_user_name";
const wchar_t* c_param_proxy_password = L"proxy_password";
const wchar_t* c_param_cache_enabled = L"cache_enabled";
const wchar_t* c_param_cache_max_size = L"cache_max_size";
const wchar_t* c_param_cache_dir = L"cache_dir";

wstring get_plugin_key_name() {
  return add_trailing_slash(Far::get_root_key_name()) + c_plugin_key_name;
}

unsigned Options::get_int(const wchar_t* name, unsigned def_value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_QUERY_VALUE);
  return plugin_key.query_int(name, def_value);
}

bool Options::get_bool(const wchar_t* name, bool def_value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_QUERY_VALUE);
  return plugin_key.query_bool(name, def_value);
}

wstring Options::get_str(const wchar_t* name, const wstring& def_value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_QUERY_VALUE);
  return plugin_key.query_str(name, def_value);
}

void Options::set_int(const wchar_t* name, unsigned value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE);
  plugin_key.set_int(name, value);
}

void Options::set_bool(const wchar_t* name, bool value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE);
  plugin_key.set_bool(name, value);
}

void Options::set_str(const wchar_t* name, const wstring& value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE);
  plugin_key.set_str(name, value);
}

void Options::load() {
  use_full_install_ui = get_bool(c_param_use_full_install_ui);
  update_stable_builds = get_bool(c_param_update_stable_builds);
  logged_install = get_bool(c_param_logged_install, true);
  install_properties = get_str(c_param_install_properties);
  http.use_proxy = get_bool(c_param_use_proxy);
  http.proxy_server = get_str(c_param_proxy_server);
  http.proxy_port = get_int(c_param_proxy_port);
  http.proxy_auth_scheme = get_int(c_param_proxy_auth_scheme);
  http.proxy_user_name = get_str(c_param_proxy_user_name);
  http.proxy_password = get_str(c_param_proxy_password);
  cache_enabled = get_bool(c_param_cache_enabled, false);
  cache_max_size = get_int(c_param_cache_max_size, 2);
  cache_dir = get_str(c_param_cache_dir, L"%TEMP%");
};

void Options::save() {
  set_bool(c_param_use_full_install_ui, use_full_install_ui);
  set_bool(c_param_update_stable_builds, update_stable_builds);
  set_bool(c_param_logged_install, logged_install);
  set_str(c_param_install_properties, install_properties);
  set_bool(c_param_use_proxy, http.use_proxy);
  set_str(c_param_proxy_server, http.proxy_server);
  set_int(c_param_proxy_port, http.proxy_port);
  set_int(c_param_proxy_auth_scheme, http.proxy_auth_scheme);
  set_str(c_param_proxy_user_name, http.proxy_user_name);
  set_str(c_param_proxy_password, http.proxy_password);
  set_bool(c_param_cache_enabled, cache_enabled);
  set_int(c_param_cache_max_size, cache_max_size);
  set_str(c_param_cache_dir, cache_dir);
}
