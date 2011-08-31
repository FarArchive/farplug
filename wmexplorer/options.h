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

struct PluginOptions {
  // plugin configuration
  bool add_to_plugin_menu;
  bool add_to_disk_menu;
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
  UnicodeString key_attr;
  UnicodeString key_execute;
  UnicodeString key_hide_rom_files;
  PluginOptions();
};

void load_plugin_options(PluginOptions& options);
void save_plugin_options(const PluginOptions& options);
void save_def_option_values(const PluginOptions& options);
UnicodeString load_last_dir(const UnicodeString& id);
void save_last_dir(const UnicodeString& id, const UnicodeString& dir);

bool virt_key_from_name(const UnicodeString& name, unsigned& key, unsigned& state);

#endif // _OPTIONS_H

