#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "options.hpp"

Options g_options;

const wchar_t* c_plugin_key_name = L"arclite";

const wchar_t* c_param_plugin_prefix = L"plugin_prefix";
const wchar_t* c_param_max_check_size = L"max_check_size";
const wchar_t* c_param_smart_path = L"smart_path";
const wchar_t* c_param_extract_ignore_errors = L"extract_ignore_errors";
const wchar_t* c_param_extract_overwrite = L"extract_overwrite";
const wchar_t* c_param_update_arc_format_name = L"update_arc_type";
const wchar_t* c_param_update_level = L"update_level";
const wchar_t* c_param_update_method = L"update_method";
const wchar_t* c_param_update_solid = L"update_solid";
const wchar_t* c_param_update_encrypt_header = L"update_encrypt_header";
const wchar_t* c_param_update_create_sfx = L"update_create_sfx";
const wchar_t* c_param_update_sfx_module_idx = L"update_sfx_module_idx";
const wchar_t* c_param_update_ignore_errors = L"update_ignore_errors";
const wchar_t* c_param_panel_view_mode = L"panel_view_mode";
const wchar_t* c_param_panel_sort_mode = L"panel_sort_mode";
const wchar_t* c_param_panel_reverse_sort = L"panel_reverse_sort";
const wchar_t* c_param_use_include_masks = L"use_include_masks";
const wchar_t* c_param_include_masks = L"include_masks";
const wchar_t* c_param_use_exclude_masks = L"use_exclude_masks";
const wchar_t* c_param_exclude_masks = L"exclude_masks";

const wchar_t* c_def_plugin_prefix = L"arc";
const unsigned c_def_max_check_size = 1 << 20;
const bool c_def_smart_path = true;
const bool c_def_extract_ignore_errors = false;
const unsigned c_def_extract_overwrite = 0;
const wchar_t* c_def_update_arc_format_name = L"7z";
const unsigned c_def_update_level = 5;
const wchar_t* c_def_update_method = L"LZMA";
const bool c_def_update_solid = true;
const bool c_def_update_encrypt_header = true;
const bool c_def_update_create_sfx = false;
const unsigned c_def_update_sfx_module_idx = 0;
const bool c_def_update_ignore_errors = false;
const unsigned c_def_panel_view_mode = 2;
const unsigned c_def_panel_sort_mode = SM_NAME;
const bool c_def_panel_reverse_sort = false;
const bool c_def_use_include_masks = false;
const wchar_t* c_def_include_masks = L"";
const bool c_def_use_exclude_masks = false;
const wchar_t* c_def_exclude_masks = L"";

wstring get_plugin_key_name() {
  return add_trailing_slash(Far::get_root_key_name()) + c_plugin_key_name;
}

unsigned Options::get_int(const wchar_t* name, unsigned def_value) {
  unsigned value;
  if (plugin_key.query_int_nt(value, name))
    return value;
  else
    return def_value;
}

bool Options::get_bool(const wchar_t* name, bool def_value) {
  bool value;
  if (plugin_key.query_bool_nt(value, name))
    return value;
  else
    return def_value;
}

wstring Options::get_str(const wchar_t* name, const wstring& def_value) {
  wstring value;
  if (plugin_key.query_str_nt(value, name))
    return value;
  else
    return def_value;
}

void Options::set_int(const wchar_t* name, unsigned value, unsigned def_value) {
  if (value == def_value)
    plugin_key.delete_value_nt(name);
  else
    plugin_key.set_int_nt(name, value);
}

void Options::set_bool(const wchar_t* name, bool value, bool def_value) {
  if (value == def_value)
    plugin_key.delete_value_nt(name);
  else
    plugin_key.set_bool_nt(name, value);
}

void Options::set_str(const wchar_t* name, const wstring& value, const wstring& def_value) {
  if (value == def_value)
    plugin_key.delete_value_nt(name);
  else
    plugin_key.set_str_nt(name, value);
}

void Options::load() {
  plugin_key.open_nt(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_QUERY_VALUE, true);
  plugin_prefix = get_str(c_param_plugin_prefix, c_def_plugin_prefix);
  max_check_size = get_int(c_param_max_check_size, c_def_max_check_size);
  smart_path = get_bool(c_param_smart_path, c_def_smart_path);
  extract_ignore_errors = get_bool(c_param_extract_ignore_errors, c_def_extract_ignore_errors);
  extract_overwrite = get_int(c_param_extract_overwrite, c_def_extract_overwrite);
  update_arc_format_name = get_str(c_param_update_arc_format_name, c_def_update_arc_format_name);
  update_level = get_int(c_param_update_level, c_def_update_level);
  update_method = get_str(c_param_update_method, c_def_update_method);
  update_solid = get_bool(c_param_update_solid, c_def_update_solid);
  update_encrypt_header = get_bool(c_param_update_encrypt_header, c_def_update_encrypt_header);
  update_create_sfx = get_bool(c_param_update_create_sfx, c_def_update_create_sfx);
  update_sfx_module_idx = get_int(c_param_update_sfx_module_idx, c_def_update_sfx_module_idx);
  update_ignore_errors = get_bool(c_param_update_ignore_errors, c_def_update_ignore_errors);
  panel_view_mode = get_int(c_param_panel_view_mode, c_def_panel_view_mode);
  panel_sort_mode = get_int(c_param_panel_sort_mode, c_def_panel_sort_mode);
  panel_reverse_sort = get_bool(c_param_panel_reverse_sort, c_def_panel_reverse_sort);
  use_include_masks = get_bool(c_param_use_include_masks, c_def_use_include_masks);
  include_masks = get_str(c_param_include_masks, c_def_include_masks);
  use_exclude_masks = get_bool(c_param_use_exclude_masks, c_def_use_exclude_masks);
  exclude_masks = get_str(c_param_exclude_masks, c_def_exclude_masks);
};

void Options::save() {
  plugin_key.open_nt(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE, true);
  set_str(c_param_plugin_prefix, plugin_prefix, c_def_plugin_prefix);
  set_int(c_param_max_check_size, max_check_size, c_def_max_check_size);
  set_bool(c_param_smart_path, smart_path, c_def_smart_path);
  set_bool(c_param_extract_ignore_errors, extract_ignore_errors, c_def_extract_ignore_errors);
  set_int(c_param_extract_overwrite, extract_overwrite, c_def_extract_overwrite);
  set_str(c_param_update_arc_format_name, update_arc_format_name, c_def_update_arc_format_name);
  set_int(c_param_update_level, update_level, c_def_update_level);
  set_str(c_param_update_method, update_method, c_def_update_method);
  set_bool(c_param_update_solid, update_solid, c_def_update_solid);
  set_bool(c_param_update_encrypt_header, update_encrypt_header, c_def_update_encrypt_header);
  set_bool(c_param_update_create_sfx, update_create_sfx, c_def_update_create_sfx);
  set_int(c_param_update_sfx_module_idx, update_sfx_module_idx, c_def_update_sfx_module_idx);
  set_bool(c_param_update_ignore_errors, update_ignore_errors, c_def_update_ignore_errors);
  set_int(c_param_panel_view_mode, panel_view_mode, c_def_panel_view_mode);
  set_int(c_param_panel_sort_mode, panel_sort_mode, c_def_panel_sort_mode);
  set_bool(c_param_panel_reverse_sort, panel_reverse_sort, c_def_panel_reverse_sort);
  set_bool(c_param_use_include_masks, use_include_masks, c_def_use_include_masks);
  set_str(c_param_include_masks, include_masks, c_def_include_masks);
  set_bool(c_param_use_exclude_masks, use_exclude_masks, c_def_use_exclude_masks);
  set_str(c_param_exclude_masks, exclude_masks, c_def_exclude_masks);
}
