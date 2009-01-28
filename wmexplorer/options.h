#ifndef _OPTIONS_H
#define _OPTIONS_H

enum ShowStatsOption {
  ssoAlways = 0,
  ssoNever = 1,
  ssoIfError = 2,
};

enum OverwriteOption {
  ooSkip = 0,
  ooOverwrite = 1,
  ooAsk = 2,
};

enum AccessMethod {
  amRapi = 0,
  amRapi2 = 1,
};

enum DevType {
  dtPDA = 0,
  dtSmartPhone = 1,
};

struct KeyOption {
  unsigned vcode;
  unsigned state;
  KeyOption() {
  }
  KeyOption(unsigned vcode, unsigned state): vcode(vcode), state(state) {
  }
};

struct PluginOptions {
  // plugin configuration
  bool add_to_plugin_menu;
  bool add_to_disk_menu;
  int disk_menu_number;
  AccessMethod access_method;
  bool hide_copy_dlg;
  bool save_last_dir;
  unsigned copy_buf_size;
  bool hide_rom_files;
  bool exit_on_dot_dot;
  UnicodeString prefix;
  bool show_free_space;
  // default option values
  bool ignore_errors;
  OverwriteOption overwrite;
  ShowStatsOption show_stats;
  bool use_file_filters;
  bool save_def_values;
  // hidden options
  DevType last_dev_type;
  // plugin keys
  KeyOption key_attr;
  KeyOption key_execute;
  KeyOption key_hide_rom_files;
};

int get_int_option(const wchar_t* name, int def_value);
bool get_bool_option(const wchar_t* name, bool def_value);
UnicodeString get_str_option(const wchar_t* name, const UnicodeString& def_value);
KeyOption get_key_option(const wchar_t* name, const KeyOption& def_key);
void set_int_option(const wchar_t* name, int value);
void set_bool_option(const wchar_t* name, bool value);
void set_str_option(const wchar_t* name, const UnicodeString& value);

void load_plugin_options(PluginOptions& options);
void save_plugin_options(const PluginOptions& options);
void save_def_option_values(const PluginOptions& options);

bool virt_key_from_name(const UnicodeString& name, unsigned& key, unsigned& state);

#endif // _OPTIONS_H

