#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "options.hpp"

Options g_options;

const wchar_t* c_plugin_key_name = L"arclite";

const wchar_t* c_param_max_check_size = L"max_check_size";
const wchar_t* c_param_extract_ignore_errors = L"extract_ignore_errors";
const wchar_t* c_param_extract_overwrite = L"extract_overwrite";
const wchar_t* c_param_update_arc_type = L"update_arc_type";
const wchar_t* c_param_update_level = L"update_level";
const wchar_t* c_param_update_method = L"update_method";

const unsigned c_def_max_check_size = 1 << 20;
const bool c_def_extract_ignore_errors = false;
const unsigned c_def_extract_overwrite = 0;
const wchar_t* c_def_update_arc_type = L"7z";
const unsigned c_def_update_level = 5;
const wchar_t* c_def_update_method = L"LZMA";

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

void Options::set_int(const wchar_t* name, unsigned value, unsigned def_value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE);
  if (value == def_value)
    IGNORE_ERRORS(plugin_key.delete_value(name))
  else
    plugin_key.set_int(name, value);
}

void Options::set_bool(const wchar_t* name, bool value, bool def_value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE);
  if (value == def_value)
    IGNORE_ERRORS(plugin_key.delete_value(name))
  else
    plugin_key.set_bool(name, value);
}

void Options::set_str(const wchar_t* name, const wstring& value, const wstring& def_value) {
  Key plugin_key(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE);
  if (value == def_value)
    IGNORE_ERRORS(plugin_key.delete_value(name))
  else
    plugin_key.set_str(name, value);
}

void Options::load() {
  max_check_size = get_int(c_param_max_check_size, c_def_max_check_size);
  extract_ignore_errors = get_bool(c_param_extract_ignore_errors, c_def_extract_ignore_errors);
  extract_overwrite = get_int(c_param_extract_overwrite, c_def_extract_overwrite);
  update_arc_type = get_str(c_param_update_arc_type, c_def_update_arc_type);
  update_level = get_int(c_param_update_level, c_def_update_level);
  update_method = get_str(c_param_update_method, c_def_update_method);
};

void Options::save() {
  set_int(c_param_max_check_size, max_check_size);
  set_bool(c_param_extract_ignore_errors, extract_ignore_errors);
  set_int(c_param_extract_overwrite, extract_overwrite);
  set_str(c_param_update_arc_type, update_arc_type);
  set_int(c_param_update_level, update_level);
  set_str(c_param_update_method, update_method);
}
