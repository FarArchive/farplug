#pragma once

extern const wchar_t* c_plugin_key_name;

struct Options {
  bool handle_create;
  bool handle_commands;
  wstring plugin_prefix;
  unsigned max_check_size;
  // extract
  bool extract_ignore_errors;
  unsigned extract_overwrite;
  unsigned extract_separate_dir;
  // update
  wstring update_arc_format_name;
  unsigned update_level;
  wstring update_method;
  bool update_solid;
  bool update_show_password;
  bool update_encrypt_header;
  unsigned update_sfx_module_idx;
  wstring update_volume_size;
  bool update_ignore_errors;
  // panel mode
  unsigned panel_view_mode;
  unsigned panel_sort_mode;
  bool panel_reverse_sort;
  // masks
  bool use_include_masks;
  wstring include_masks;
  bool use_exclude_masks;
  wstring exclude_masks;
  // profiles
  void load();
  void save() const;
};

extern Options g_options;
extern UpdateProfiles g_profiles;
