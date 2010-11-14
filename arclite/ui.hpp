#pragma once

const wchar_t** get_size_suffixes();
const wchar_t** get_speed_suffixes();

bool password_dialog(wstring& password, const wstring& arc_path);

enum OverwriteAction { oaYes, oaYesAll, oaNo, oaNoAll, oaCancel };
struct OverwriteFileInfo {
  bool is_dir;
  unsigned __int64 size;
  FILETIME mtime;
};
OverwriteAction overwrite_dialog(const wstring& file_path, const OverwriteFileInfo& src_file_info, const OverwriteFileInfo& dst_file_info);

bool extract_dialog(ExtractOptions& options);

void show_error_log(const ErrorLog& error_log);

bool update_dialog(bool new_arc, UpdateOptions& options, UpdateProfiles& profiles);

struct PluginSettings {
  bool handle_create;
  bool handle_commands;
  bool use_include_masks;
  wstring include_masks;
  bool use_exclude_masks;
  wstring exclude_masks;
};

bool settings_dialog(PluginSettings& settings);

void attr_dialog(const AttrList& attr_list);

bool sfx_convert_dialog(wstring& sfx_module_idx);
