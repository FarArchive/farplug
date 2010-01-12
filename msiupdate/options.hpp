#pragma once

extern const wchar_t* c_plugin_key_name;

struct HttpOptions {
  bool use_proxy;
  wstring proxy_server;
  unsigned proxy_port;
  unsigned proxy_auth_scheme;
  wstring proxy_user_name;
  wstring proxy_password;
};

class Options {
public:
  static unsigned get_int(const wchar_t* name, unsigned def_value = 0);
  static bool get_bool(const wchar_t* name, bool def_value = false);
  static wstring get_str(const wchar_t* name, const wstring& def_value = wstring());
  static void set_int(const wchar_t* name, unsigned value);
  static void set_bool(const wchar_t* name, bool value);
  static void set_str(const wchar_t* name, const wstring& value);
  unsigned last_check_time;
  unsigned last_check_version;
  bool use_full_install_ui;
  bool update_stable_builds;
  bool logged_install;
  wstring install_properties;
  HttpOptions http;
  bool cache_enabled;
  unsigned cache_max_size;
  wstring cache_dir;
  void load();
  void save();
};

extern Options g_options;
