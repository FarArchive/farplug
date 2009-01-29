#ifndef _UTIL_H
#define _UTIL_H

#ifdef DEBUG
#define LOG_MSG(msg) OutputDebugStringA((msg + '\n').data());
#define LOG_UMSG(msg) OutputDebugStringW((msg + '\n').data());
#define LOG_ERR(e) OutputDebugStringW(UnicodeString::format(L"File: %s Line: %u \"%s\"\n", oem_to_unicode(e.file).data(), e.line, e.message().data()).data());
#else // DEBUG
#define LOG_MSG(msg)
#define LOG_UMSG(msg)
#define LOG_ERR(e) (e)
#endif // DEBUG

void unicode_to_oem(AnsiString& oem_str, const UnicodeString& u_str);
void oem_to_unicode(UnicodeString& u_str, const AnsiString& oem_str);
AnsiString unicode_to_oem(const UnicodeString& u_str);
UnicodeString oem_to_unicode(const AnsiString& oem_str);
void unicode_to_ansi(AnsiString& a_str, const UnicodeString& u_str);
void ansi_to_unicode(UnicodeString& u_str, const AnsiString& a_str);
AnsiString unicode_to_ansi(const UnicodeString& u_str);
UnicodeString ansi_to_unicode(const AnsiString& a_str);

UnicodeString word_wrap(const UnicodeString& message, unsigned wrap_bound);
UnicodeString format_data_size(unsigned __int64 size, const ObjectArray<UnicodeString>& suffixes);
UnicodeString format_time(unsigned __int64 t /* ms */);
UnicodeString fit_str(const UnicodeString& path, unsigned size);
UnicodeString center(const UnicodeString& str, unsigned width);
unsigned __int64 mul_div(unsigned __int64 a, unsigned __int64 b, unsigned __int64 c);

#ifdef FARAPI17
#  ifdef _WIN64
#    define PLUGIN_TYPE L" x64"
#  else
#    define PLUGIN_TYPE L""
#  endif
#endif
#ifdef FARAPI18
#  ifdef _WIN64
#    define PLUGIN_TYPE L" uni x64"
#  else
#    define PLUGIN_TYPE L" uni"
#  endif
#endif

struct ModuleVersion {
  unsigned major;
  unsigned minor;
  unsigned patch;
  unsigned revision;
};
ModuleVersion get_module_version(HINSTANCE module);
const UnicodeString extract_file_name(const UnicodeString& file_path);
void unquote(UnicodeString& str);

#ifdef FARAPI17
void encode_fn(UnicodeString& dst, const UnicodeString& src);
UnicodeString encode_fn(const UnicodeString& src);
void decode_fn(UnicodeString& dst, const UnicodeString& src);
UnicodeString decode_fn(const UnicodeString& src);
#endif

UnicodeString make_temp_file();
UnicodeString format_file_time(const FILETIME& file_time);

int far_control_int(HANDLE h_panel, int command, int param);
int far_control_ptr(HANDLE h_panel, int command, const void* param);
PluginPanelItem* far_get_panel_item(HANDLE h_panel, int index, const PanelInfo& pi);
PluginPanelItem* far_get_selected_panel_item(HANDLE h_panel, int index, const PanelInfo& pi);

#endif // _UTIL_H
