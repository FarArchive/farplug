#pragma once

class Options {
private:
  unsigned get_int(const wchar_t* name, unsigned def_value);
  bool get_bool(const wchar_t* name, bool def_value);
  wstring get_str(const wchar_t* name, const wstring& def_value);
  void set_int(const wchar_t* name, unsigned value);
  void set_bool(const wchar_t* name, bool value);
  void set_str(const wchar_t* name, const wstring& value);
public:
  unsigned last_check_time;
  unsigned last_check_version;
  bool use_full_install_ui;
  bool update_stable_builds;
  bool logged_install;
  void load();
  void save();
};

extern Options g_options;
