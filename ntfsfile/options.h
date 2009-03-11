#pragma once

/* content analysis options structure */
struct ContentOptions {
  bool compression;
  bool crc32;
  bool md5;
  bool sha1;
  bool ed2k;
};

struct FilePanelMode {
  UnicodeString col_types;
  UnicodeString col_widths;
  UnicodeString status_col_types;
  UnicodeString status_col_widths;
  bool wide;
  int sort_mode;
  int reverse_sort;
  int numeric_sort;
  int custom_sort_mode;
  bool show_streams;
  bool show_main_stream;
  bool use_highlighting;
  bool use_usn_journal;
  bool delete_usn_journal;
  bool use_cache;
  bool default_mft_mode;
};

/* plugin options */
extern bool g_use_standard_inf_units;
extern ContentOptions g_content_options;
extern FilePanelMode g_file_panel_mode;

void load_plugin_options();
void store_plugin_options();
