#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "utils.h"
#include "options.h"

extern struct PluginStartupInfo g_far;

/* plugin options */
bool g_use_standard_inf_units;
ContentOptions g_content_options;

const wchar_t* c_plugin_key_name = L"ntfsfile";

UnicodeString get_root_key_name() {
  return FARSTR_TO_UNICODE(g_far.RootKey);
}

int get_int_option(const wchar_t* name, int def_value) {
  int value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_DWORD;
      DWORD data;
      DWORD data_size = sizeof(data);
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, (LPBYTE) &data, &data_size);
      if (res == ERROR_SUCCESS) value = data;
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

bool get_bool_option(const wchar_t* name, bool def_value) {
  bool value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_DWORD;
      DWORD data;
      DWORD data_size = sizeof(data);
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, (LPBYTE) &data, &data_size);
      if (res == ERROR_SUCCESS) value = data != 0;
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

UnicodeString get_str_option(const wchar_t* name, const UnicodeString& def_value) {
  UnicodeString value = def_value;
  HKEY h_root_key, h_plugin_key;
  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, KEY_QUERY_VALUE, &h_root_key);
  if (res == ERROR_SUCCESS) {
    res = RegOpenKeyExW(h_root_key, c_plugin_key_name, 0, KEY_QUERY_VALUE, &h_plugin_key);
    if (res == ERROR_SUCCESS) {
      DWORD type = REG_SZ;
      DWORD data_size;
      // get string size
      res = RegQueryValueExW(h_plugin_key, name, NULL, &type, NULL, &data_size);
      if (res == ERROR_SUCCESS) {
        UnicodeString data;
        // get string value
        res = RegQueryValueExW(h_plugin_key, name, NULL, &type, (LPBYTE) data.buf(data_size / sizeof(wchar_t)), &data_size);
        if (res == ERROR_SUCCESS) {
          data.set_size(data_size / sizeof(wchar_t) - 1); // throw away terminating NULL
          value = data;
        }
      }
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
  return value;
}

void set_int_option(const wchar_t* name, int value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      DWORD data = value;
      RegSetValueExW(h_plugin_key, name, 0, REG_DWORD, (LPBYTE) &data, sizeof(data));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void set_bool_option(const wchar_t* name, bool value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      DWORD data = value ? 1 : 0;
      RegSetValueExW(h_plugin_key, name, 0, REG_DWORD, (LPBYTE) &data, sizeof(data));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void set_str_option(const wchar_t* name, const UnicodeString& value) {
  HKEY h_root_key, h_plugin_key;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, get_root_key_name().data(), 0, NULL, 0, KEY_SET_VALUE, NULL, &h_root_key, NULL);
  if (res == ERROR_SUCCESS) {
    res = RegCreateKeyExW(h_root_key, c_plugin_key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, &h_plugin_key, NULL);
    if (res == ERROR_SUCCESS) {
      // size should include terminating NULL
      RegSetValueExW(h_plugin_key, name, 0, REG_SZ, (LPBYTE) value.data(), (value.size() + 1) * sizeof(wchar_t));
      RegCloseKey(h_plugin_key);
    }
    RegCloseKey(h_root_key);
  }
}

void load_plugin_options() {
  g_use_standard_inf_units = get_bool_option(L"StandardInformationUnits", false);
  g_content_options.compression = get_bool_option(L"ContentOptionsCompression", false);
  g_content_options.crc32 = get_bool_option(L"ContentOptionsCRC32", false);
  g_content_options.md5 = get_bool_option(L"ContentOptionsMD5", false);
  g_content_options.sha1 = get_bool_option(L"ContentOptionsSHA1", false);
  g_content_options.sha256 = get_bool_option(L"ContentOptionsSHA256", false);
  g_content_options.ed2k = get_bool_option(L"ContentOptionsED2K", false);
  g_file_panel_mode.col_types = get_str_option(L"FilePanelColTypes", L"N,DSZ,RSZ,FRG,STM,LNK,MFT");
  g_file_panel_mode.col_widths = get_str_option(L"FilePanelColWidths", L"0,7,7,5,3,3,3");
  g_file_panel_mode.status_col_types = get_str_option(L"FilePanelStatusColTypes", L"NR,A,D,T");
  g_file_panel_mode.status_col_widths = get_str_option(L"FilePanelStatusColWidths", L"0,10,0,0");
  g_file_panel_mode.wide = get_bool_option(L"FilePanelWide", false);
  g_file_panel_mode.sort_mode = get_int_option(L"FilePanelSortMode", SM_UNSORTED);
  g_file_panel_mode.reverse_sort = get_int_option(L"FilePanelReverseSort", 0);
  g_file_panel_mode.numeric_sort = get_int_option(L"FilePanelNumericSort", 0);
  g_file_panel_mode.sort_dirs_first = get_int_option(L"FilePanelSortDirsFirst", 0);
  g_file_panel_mode.custom_sort_mode = get_int_option(L"CustomSortMode", 0);
  g_file_panel_mode.show_streams = get_bool_option(L"FilePanelShowStreams", false);
  g_file_panel_mode.show_main_stream = get_bool_option(L"FilePanelShowMainStream", false);
  g_file_panel_mode.use_highlighting = get_bool_option(L"FilePanelUseHighlighting", true);
  g_file_panel_mode.use_usn_journal = get_bool_option(L"FilePanelUseUsnJournal", true);
  g_file_panel_mode.delete_usn_journal = get_bool_option(L"FilePanelDeleteUsnJournal", false);
  g_file_panel_mode.use_cache = get_bool_option(L"FilePanelUseCache", true);
  g_file_panel_mode.default_mft_mode = get_bool_option(L"FilePanelDefaultMftMode", true);
  g_file_panel_mode.backward_mft_scan = get_bool_option(L"FilePanelBackwardMftScan", true);
  g_file_panel_mode.cache_dir = get_str_option(L"FilePanelCacheDir", L"%TEMP%");
  g_file_panel_mode.flat_mode_auto_off = get_bool_option(L"FilePanelFlatModeAutoOff", false);
#ifdef FARAPI17
  g_file_panel_mode.use_std_sort = get_bool_option(L"FilePanelUseStdSort", false);
#endif
  g_compress_files_params.min_file_size = get_int_option(L"CompressFilesMinFileSize", 10);
  g_compress_files_params.max_compression_ratio = get_int_option(L"CompressFilesMaxCompressionRatio", 80);
};

void store_plugin_options() {
  set_bool_option(L"ContentOptionsCompression", g_content_options.compression);
  set_bool_option(L"ContentOptionsCRC32", g_content_options.crc32);
  set_bool_option(L"ContentOptionsMD5", g_content_options.md5);
  set_bool_option(L"ContentOptionsSHA1", g_content_options.sha1);
  set_bool_option(L"ContentOptionsSHA256", g_content_options.sha256);
  set_bool_option(L"ContentOptionsED2K", g_content_options.ed2k);
  set_str_option(L"FilePanelColTypes", g_file_panel_mode.col_types);
  set_str_option(L"FilePanelColWidths", g_file_panel_mode.col_widths);
  set_str_option(L"FilePanelStatusColTypes", g_file_panel_mode.status_col_types);
  set_str_option(L"FilePanelStatusColWidths", g_file_panel_mode.status_col_widths);
  set_bool_option(L"FilePanelWide", g_file_panel_mode.wide);
  set_int_option(L"FilePanelSortMode", g_file_panel_mode.sort_mode);
  set_int_option(L"FilePanelReverseSort", g_file_panel_mode.reverse_sort);
  set_int_option(L"FilePanelNumericSort", g_file_panel_mode.numeric_sort);
  set_int_option(L"FilePanelSortDirsFirst", g_file_panel_mode.sort_dirs_first);
  set_int_option(L"CustomSortMode", g_file_panel_mode.custom_sort_mode);
  set_bool_option(L"FilePanelShowStreams", g_file_panel_mode.show_streams);
  set_bool_option(L"FilePanelShowMainStream", g_file_panel_mode.show_main_stream);
  set_bool_option(L"FilePanelUseHighlighting", g_file_panel_mode.use_highlighting);
  set_bool_option(L"FilePanelUseUsnJournal", g_file_panel_mode.use_usn_journal);
  set_bool_option(L"FilePanelDeleteUsnJournal", g_file_panel_mode.delete_usn_journal);
  set_bool_option(L"FilePanelUseCache", g_file_panel_mode.use_cache);
  set_bool_option(L"FilePanelDefaultMftMode", g_file_panel_mode.default_mft_mode);
  set_bool_option(L"FilePanelBackwardMftScan", g_file_panel_mode.backward_mft_scan);
  set_str_option(L"FilePanelCacheDir", g_file_panel_mode.cache_dir);
  set_bool_option(L"FilePanelFlatModeAutoOff", g_file_panel_mode.flat_mode_auto_off);
#ifdef FARAPI17
  set_bool_option(L"FilePanelUseStdSort", g_file_panel_mode.use_std_sort);
#endif
  set_int_option(L"CompressFilesMinFileSize", g_compress_files_params.min_file_size);
  set_int_option(L"CompressFilesMaxCompressionRatio", g_compress_files_params.max_compression_ratio);
}
