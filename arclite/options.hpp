#pragma once

extern const wchar_t* c_plugin_key_name;

class Options {
private:
  static unsigned get_int(const wchar_t* name, unsigned def_value = 0);
  static bool get_bool(const wchar_t* name, bool def_value = false);
  static wstring get_str(const wchar_t* name, const wstring& def_value = wstring());
  static void set_int(const wchar_t* name, unsigned value, unsigned def_value = 0);
  static void set_bool(const wchar_t* name, bool value, bool def_value = false);
  static void set_str(const wchar_t* name, const wstring& value, const wstring& def_value = wstring());
public:
  unsigned max_check_size;
  bool extract_ignore_errors;
  unsigned extract_overwrite;
  wstring update_arc_format_name;
  unsigned update_level;
  wstring update_method;
  bool update_solid;
  bool update_encrypt_header;
  // panel mode
  unsigned panel_view_mode;
  unsigned panel_sort_mode;
  bool panel_reverse_sort;
  // masks
  bool use_include_masks;
  wstring include_masks;
  bool use_exclude_masks;
  wstring exclude_masks;
public:
  void load();
  void save();
};

extern Options g_options;
