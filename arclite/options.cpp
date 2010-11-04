#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common.hpp"
#include "archive.hpp"
#include "options.hpp"


class OptionsKey: public Key {
public:
  unsigned get_int(const wchar_t* name, unsigned def_value) {
    unsigned value;
    if (query_int_nt(value, name))
      return value;
    else
      return def_value;
  }

  bool get_bool(const wchar_t* name, bool def_value) {
    bool value;
    if (query_bool_nt(value, name))
      return value;
    else
      return def_value;
  }

  wstring get_str(const wchar_t* name, const wstring& def_value) {
    wstring value;
    if (query_str_nt(value, name))
      return value;
    else
      return def_value;
  }

  ByteVector get_binary(const wchar_t* name, const ByteVector& def_value) {
    ByteVector value;
    if (query_binary_nt(value, name))
      return value;
    else
      return def_value;
  }

  TriState get_tri_state(const wchar_t* name, TriState def_value) {
    unsigned value;
    if (query_int_nt(value, name))
      return static_cast<TriState>(value);
    else
      return def_value;
  }

  void set_int(const wchar_t* name, unsigned value, unsigned def_value) {
    if (value == def_value)
      delete_value_nt(name);
    else
      set_int_nt(name, value);
  }

  void set_bool(const wchar_t* name, bool value, bool def_value) {
    if (value == def_value)
      delete_value_nt(name);
    else
      set_bool_nt(name, value);
  }

  void set_str(const wchar_t* name, const wstring& value, const wstring& def_value) {
    if (value == def_value)
      delete_value_nt(name);
    else
      set_str_nt(name, value);
  }

  void set_binary(const wchar_t* name, const ByteVector& value, const ByteVector& def_value) {
    if (value == def_value)
      delete_value_nt(name);
    else
      set_binary_nt(name, value.data(), static_cast<unsigned>(value.size()));
  }

  void set_tri_state(const wchar_t* name, TriState value, TriState def_value) {
    if (value == def_value)
      delete_value_nt(name);
    else
      set_int_nt(name, static_cast<unsigned>(value));
  }
};

Options g_options;
UpdateProfiles g_profiles;

const wchar_t* c_plugin_key_name = L"arclite";
const wchar_t* c_profiles_key_name = L"profiles";

wstring get_plugin_key_name() {
  return add_trailing_slash(Far::get_root_key_name()) + c_plugin_key_name;
}

wstring get_profiles_key_name() {
  return add_trailing_slash(add_trailing_slash(Far::get_root_key_name()) + c_plugin_key_name) + c_profiles_key_name;
}

wstring get_profile_key_name(const wstring& name) {
  return add_trailing_slash(add_trailing_slash(add_trailing_slash(Far::get_root_key_name()) + c_plugin_key_name )+ c_profiles_key_name) + name;
}

const wchar_t* c_param_handle_create = L"handle_create";
const wchar_t* c_param_handle_commands = L"handle_commands";
const wchar_t* c_param_plugin_prefix = L"plugin_prefix";
const wchar_t* c_param_max_check_size = L"max_check_size";
const wchar_t* c_param_extract_ignore_errors = L"extract_ignore_errors";
const wchar_t* c_param_extract_overwrite = L"extract_overwrite";
const wchar_t* c_param_extract_separate_dir = L"extract_separate_dir";
const wchar_t* c_param_update_arc_format_name = L"update_arc_type";
const wchar_t* c_param_update_level = L"update_level";
const wchar_t* c_param_update_method = L"update_method";
const wchar_t* c_param_update_solid = L"update_solid";
const wchar_t* c_param_update_show_password = L"update_show_password";
const wchar_t* c_param_update_encrypt_header = L"update_encrypt_header";
const wchar_t* c_param_update_sfx_module = L"update_sfx_module";
const wchar_t* c_param_update_volume_size = L"update_volume_size";
const wchar_t* c_param_update_ignore_errors = L"update_ignore_errors";
const wchar_t* c_param_panel_view_mode = L"panel_view_mode";
const wchar_t* c_param_panel_sort_mode = L"panel_sort_mode";
const wchar_t* c_param_panel_reverse_sort = L"panel_reverse_sort";
const wchar_t* c_param_use_include_masks = L"use_include_masks";
const wchar_t* c_param_include_masks = L"include_masks";
const wchar_t* c_param_use_exclude_masks = L"use_exclude_masks";
const wchar_t* c_param_exclude_masks = L"exclude_masks";

const bool c_def_handle_create = true;
const bool c_def_handle_commands = true;
const wchar_t* c_def_plugin_prefix = L"arc";
const unsigned c_def_max_check_size = 1 << 20;
const bool c_def_extract_ignore_errors = false;
const TriState c_def_extract_overwrite = triUndef;
const TriState c_def_extract_separate_dir = triUndef;
const wchar_t* c_def_update_arc_format_name = L"7z";
const unsigned c_def_update_level = 5;
const wchar_t* c_def_update_method = L"LZMA";
const bool c_def_update_solid = true;
const bool c_def_update_show_password = false;
const TriState c_def_update_encrypt_header = triUndef;
const wchar_t* c_def_update_sfx_module = L"";
const wchar_t* c_def_update_volume_size = L"";
const bool c_def_update_ignore_errors = false;
const unsigned c_def_panel_view_mode = 2;
const unsigned c_def_panel_sort_mode = SM_NAME;
const bool c_def_panel_reverse_sort = false;
const bool c_def_use_include_masks = false;
const wchar_t* c_def_include_masks = L"";
const bool c_def_use_exclude_masks = false;
const wchar_t* c_def_exclude_masks = L"";

void Options::load() {
  OptionsKey key;
  key.open_nt(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_QUERY_VALUE, false);
  handle_create = key.get_bool(c_param_handle_create, c_def_handle_create);
  handle_commands = key.get_bool(c_param_handle_commands, c_def_handle_commands);
  plugin_prefix = key.get_str(c_param_plugin_prefix, c_def_plugin_prefix);
  max_check_size = key.get_int(c_param_max_check_size, c_def_max_check_size);
  extract_ignore_errors = key.get_bool(c_param_extract_ignore_errors, c_def_extract_ignore_errors);
  extract_overwrite = key.get_tri_state(c_param_extract_overwrite, c_def_extract_overwrite);
  extract_separate_dir = key.get_tri_state(c_param_extract_separate_dir, c_def_extract_separate_dir);
  update_arc_format_name = key.get_str(c_param_update_arc_format_name, c_def_update_arc_format_name);
  update_level = key.get_int(c_param_update_level, c_def_update_level);
  update_method = key.get_str(c_param_update_method, c_def_update_method);
  update_solid = key.get_bool(c_param_update_solid, c_def_update_solid);
  update_show_password = key.get_bool(c_param_update_show_password, c_def_update_show_password);
  update_encrypt_header = key.get_tri_state(c_param_update_encrypt_header, c_def_update_encrypt_header);
  update_sfx_module = key.get_str(c_param_update_sfx_module, c_def_update_sfx_module);
  update_volume_size = key.get_str(c_param_update_volume_size, c_def_update_volume_size);
  update_ignore_errors = key.get_bool(c_param_update_ignore_errors, c_def_update_ignore_errors);
  panel_view_mode = key.get_int(c_param_panel_view_mode, c_def_panel_view_mode);
  panel_sort_mode = key.get_int(c_param_panel_sort_mode, c_def_panel_sort_mode);
  panel_reverse_sort = key.get_bool(c_param_panel_reverse_sort, c_def_panel_reverse_sort);
  use_include_masks = key.get_bool(c_param_use_include_masks, c_def_use_include_masks);
  include_masks = key.get_str(c_param_include_masks, c_def_include_masks);
  use_exclude_masks = key.get_bool(c_param_use_exclude_masks, c_def_use_exclude_masks);
  exclude_masks = key.get_str(c_param_exclude_masks, c_def_exclude_masks);
};

void Options::save() const {
  OptionsKey key;
  key.open_nt(HKEY_CURRENT_USER, get_plugin_key_name().c_str(), KEY_SET_VALUE, true);
  key.set_bool(c_param_handle_create, handle_create, c_def_handle_create);
  key.set_bool(c_param_handle_commands, handle_commands, c_def_handle_commands);
  key.set_str(c_param_plugin_prefix, plugin_prefix, c_def_plugin_prefix);
  key.set_int(c_param_max_check_size, max_check_size, c_def_max_check_size);
  key.set_bool(c_param_extract_ignore_errors, extract_ignore_errors, c_def_extract_ignore_errors);
  key.set_tri_state(c_param_extract_overwrite, extract_overwrite, c_def_extract_overwrite);
  key.set_tri_state(c_param_extract_separate_dir, extract_separate_dir, c_def_extract_separate_dir);
  key.set_str(c_param_update_arc_format_name, update_arc_format_name, c_def_update_arc_format_name);
  key.set_int(c_param_update_level, update_level, c_def_update_level);
  key.set_str(c_param_update_method, update_method, c_def_update_method);
  key.set_bool(c_param_update_solid, update_solid, c_def_update_solid);
  key.set_bool(c_param_update_show_password, update_show_password, c_def_update_show_password);
  key.set_tri_state(c_param_update_encrypt_header, update_encrypt_header, c_def_update_encrypt_header);
  key.set_str(c_param_update_sfx_module, update_sfx_module, c_def_update_sfx_module);
  key.set_str(c_param_update_volume_size, update_volume_size, c_def_update_volume_size);
  key.set_bool(c_param_update_ignore_errors, update_ignore_errors, c_def_update_ignore_errors);
  key.set_int(c_param_panel_view_mode, panel_view_mode, c_def_panel_view_mode);
  key.set_int(c_param_panel_sort_mode, panel_sort_mode, c_def_panel_sort_mode);
  key.set_bool(c_param_panel_reverse_sort, panel_reverse_sort, c_def_panel_reverse_sort);
  key.set_bool(c_param_use_include_masks, use_include_masks, c_def_use_include_masks);
  key.set_str(c_param_include_masks, include_masks, c_def_include_masks);
  key.set_bool(c_param_use_exclude_masks, use_exclude_masks, c_def_use_exclude_masks);
  key.set_str(c_param_exclude_masks, exclude_masks, c_def_exclude_masks);
}

const wchar_t* c_param_profile_arc_path = L"arc_path";
const wchar_t* c_param_profile_arc_type = L"arc_type";
const wchar_t* c_param_profile_level = L"level";
const wchar_t* c_param_profile_method = L"method";
const wchar_t* c_param_profile_solid = L"solid";
const wchar_t* c_param_profile_password = L"password";
const wchar_t* c_param_profile_show_password = L"show_password";
const wchar_t* c_param_profile_encrypt = L"encrypt";
const wchar_t* c_param_profile_encrypt_header = L"encrypt_header";
const wchar_t* c_param_profile_encrypt_header_defined = L"encrypt_header_defined";
const wchar_t* c_param_profile_create_sfx = L"create_sfx";
const wchar_t* c_param_profile_sfx_module = L"sfx_module";
const wchar_t* c_param_profile_enable_volumes = L"enable_volumes";
const wchar_t* c_param_profile_volume_size = L"volume_size";
const wchar_t* c_param_profile_move_files = L"move_files";
const wchar_t* c_param_profile_open_shared = L"open_shared";
const wchar_t* c_param_profile_ignore_errors = L"ignore_errors";
const wchar_t* c_param_profile_advanced = L"advanced";

const wchar_t* c_def_profile_arc_path = L"";
const ArcType c_def_profile_arc_type = c_7z;
const unsigned c_def_profile_level = 5;
const wchar_t* c_def_profile_method = L"LZMA";
const bool c_def_profile_solid = true;
const wchar_t* c_def_profile_password = L"";
const bool c_def_profile_show_password = false;
const bool c_def_profile_encrypt = false;
const TriState c_def_profile_encrypt_header = triUndef;
const bool c_def_profile_create_sfx = false;
const wchar_t* c_def_profile_sfx_module = L"";
const bool c_def_profile_enable_volumes = false;
const wchar_t* c_def_profile_volume_size = L"";
const bool c_def_profile_move_files = false;
const bool c_def_profile_open_shared = false;
const bool c_def_profile_ignore_errors = false;
const wchar_t* c_def_profile_advanced = L"";

UpdateOptions::UpdateOptions():
  arc_path(c_def_profile_arc_path),
  arc_type(c_def_profile_arc_type),
  level(c_def_profile_level),
  method(c_def_profile_method),
  solid(c_def_profile_solid),
  password(c_def_profile_password),
  show_password(c_def_profile_show_password),
  encrypt(c_def_profile_encrypt),
  encrypt_header(c_def_profile_encrypt_header),
  create_sfx(c_def_profile_create_sfx),
  sfx_module(c_def_profile_sfx_module),
  enable_volumes(c_def_profile_enable_volumes),
  volume_size(c_def_profile_volume_size),
  move_files(c_def_profile_move_files),
  open_shared(c_def_profile_open_shared),
  ignore_errors(c_def_profile_ignore_errors),
  advanced(c_def_profile_advanced)
{}

void UpdateProfiles::load() {
  clear();
  Key profiles_key;
  if (profiles_key.open_nt(HKEY_CURRENT_USER, get_profiles_key_name().c_str(), KEY_ENUMERATE_SUB_KEYS, false)) {
    vector<wstring> profile_names = profiles_key.enum_sub_keys();
    for (unsigned i = 0; i < profile_names.size(); i++) {
      UpdateProfile profile;
      profile.name = profile_names[i];
      OptionsKey key;
      key.open_nt(HKEY_CURRENT_USER, get_profile_key_name(profile.name).c_str(), KEY_QUERY_VALUE, false);
#define GET_VALUE(name, type) profile.options.name = key.get_##type(c_param_profile_##name, c_def_profile_##name)
      GET_VALUE(arc_type, binary);
      GET_VALUE(level, int);
      GET_VALUE(method, str);
      GET_VALUE(solid, bool);
      GET_VALUE(password, str);
      GET_VALUE(show_password, bool);
      GET_VALUE(encrypt, bool);
      GET_VALUE(encrypt_header, tri_state);
      GET_VALUE(create_sfx, bool);
      GET_VALUE(sfx_module, str);
      GET_VALUE(enable_volumes, bool);
      GET_VALUE(volume_size, str);
      GET_VALUE(move_files, bool);
      GET_VALUE(open_shared, bool);
      GET_VALUE(ignore_errors, bool);
      GET_VALUE(advanced, str);
#undef GET_VALUE
      push_back(profile);
    }
  }
}

void UpdateProfiles::save() const {
  Key profiles_key;
  if (profiles_key.open_nt(HKEY_CURRENT_USER, get_profiles_key_name().c_str(), KEY_ENUMERATE_SUB_KEYS, true)) {
    vector<wstring> profile_names = profiles_key.enum_sub_keys();
    for (unsigned i = 0; i < profile_names.size(); i++) {
      profiles_key.delete_sub_key(profile_names[i].c_str());
    }
  }
  for_each(begin(), end(), [&] (const UpdateProfile& profile) {
    OptionsKey key;
    key.open_nt(HKEY_CURRENT_USER, get_profile_key_name(profile.name).c_str(), KEY_SET_VALUE, true);
#define SET_VALUE(name, type) key.set_##type(c_param_profile_##name, profile.options.name, c_def_profile_##name)
    SET_VALUE(arc_type, binary);
    SET_VALUE(level, int);
    SET_VALUE(method, str);
    SET_VALUE(solid, bool);
    SET_VALUE(password, str);
    SET_VALUE(show_password, bool);
    SET_VALUE(encrypt, bool);
    SET_VALUE(encrypt_header, tri_state);
    SET_VALUE(create_sfx, bool);
    SET_VALUE(sfx_module, str);
    SET_VALUE(enable_volumes, bool);
    SET_VALUE(volume_size, str);
    SET_VALUE(move_files, bool);
    SET_VALUE(open_shared, bool);
    SET_VALUE(ignore_errors, bool);
    SET_VALUE(advanced, str);
#undef SET_VALUE
  });
}
