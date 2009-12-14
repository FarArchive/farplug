#pragma once

struct HttpOptions {
  bool use_proxy;
  wstring proxy_server;
  unsigned proxy_port;
  unsigned proxy_auth_scheme;
  wstring proxy_user_name;
  wstring proxy_password;
};

class Options {
private:
  unsigned get_int(const wchar_t* name, unsigned def_value = 0);
  bool get_bool(const wchar_t* name, bool def_value = false);
  wstring get_str(const wchar_t* name, const wstring& def_value = wstring());
  void set_int(const wchar_t* name, unsigned value);
  void set_bool(const wchar_t* name, bool value);
  void set_str(const wchar_t* name, const wstring& value);
public:
  unsigned last_check_time;
  unsigned last_check_version;
  bool use_full_install_ui;
  bool update_stable_builds;
  bool logged_install;
  HttpOptions http;
  void load();
  void save();
};

extern Options g_options;
