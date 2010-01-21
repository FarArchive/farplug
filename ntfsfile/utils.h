#pragma once

typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;
typedef __int8 s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

#ifdef DEBUG
#  define DBG_LOG(msg) OutputDebugStringW(((msg) + L'\n').data())
#else
#  define DBG_LOG(msg)
#endif

void unicode_to_oem(AnsiString& oem_str, const UnicodeString& u_str);
AnsiString unicode_to_oem(const UnicodeString& str);
void oem_to_unicode(UnicodeString& u_str, const AnsiString& oem_str);
UnicodeString oem_to_unicode(const AnsiString& str);
UnicodeString format_inf_amount(u64 size);
UnicodeString format_inf_amount_short(u64 size, bool speed = false);
UnicodeString format_time(u64 t /* ms */);
UnicodeString format_time2(u64 t /* ms */);
UnicodeString format_hex_array(const Array<u8>& a);
unsigned nod(unsigned a, unsigned b);
unsigned nok(unsigned a, unsigned b);
bool check_for_esc(void);
UnicodeString word_wrap(const UnicodeString& message, unsigned wrap_bound);
struct ModuleVersion {
  unsigned major;
  unsigned minor;
  unsigned patch;
  unsigned revision;
};
ModuleVersion get_module_version(HINSTANCE module);
UnicodeString extract_path_root(const UnicodeString& path);
UnicodeString extract_file_name(const UnicodeString& path);
UnicodeString extract_file_path(const UnicodeString& path);
UnicodeString remove_path_root(const UnicodeString& path);
bool is_root_path(const UnicodeString& path);
bool is_unc_path(const UnicodeString& path);

UnicodeString long_path(const UnicodeString& path);
UnicodeString add_trailing_slash(const UnicodeString& file_path);
UnicodeString del_trailing_slash(const UnicodeString& file_path);
int str_to_int(const UnicodeString& str);
UnicodeString int_to_str(int val);
UnicodeString center(const UnicodeString& str, unsigned width);
UnicodeString fit_str(const UnicodeString& path, unsigned size);
void unquote(UnicodeString& str);
ObjectArray<UnicodeString> split_str(const UnicodeString& str, wchar_t split_ch);

extern ObjectArray<UnicodeString> size_suffixes;
extern ObjectArray<UnicodeString> speed_suffixes;
extern ObjectArray<UnicodeString> short_size_suffixes;
void load_suffixes();
UnicodeString format_data_size(unsigned __int64 value, const ObjectArray<UnicodeString>& suffixes);

class ProgressMonitor {
private:
  HANDLE h_scr;
  UnicodeString con_title;
  unsigned __int64 t_start;
  unsigned __int64 t_curr;
  unsigned __int64 t_next;
  unsigned __int64 t_freq;
protected:
  virtual void do_update_ui() = 0;
public:
  ProgressMonitor(bool lazy = true);
  virtual ~ProgressMonitor();
  void update_ui(bool force = false);
  unsigned __int64 time_elapsed() const {
    return (t_curr - t_start) / (t_freq / 1000);
  }
};

int far_control_int(HANDLE h_panel, int command, int param);
int far_control_ptr(HANDLE h_panel, int command, const void* param);
FarStr far_get_panel_dir(HANDLE h_panel, const PanelInfo& pi);
UnicodeString far_get_full_path(const UnicodeString& file_name);
PluginPanelItem* far_get_panel_item(HANDLE h_panel, int index, const PanelInfo& pi);
PluginPanelItem* far_get_selected_panel_item(HANDLE h_panel, int index, const PanelInfo& pi);
void far_set_progress_state(TBPFLAG state);
void far_set_progress_value(unsigned __int64 completed, unsigned __int64 total);

#define BEGIN_ERROR_HANDLER try {
#define END_ERROR_HANDLER(success, failure) \
    success; \
  } \
  catch (Break&) { \
    failure; \
  } \
  catch (Error& e) { \
    error_dlg(e); \
    failure; \
  } \
  catch (std::exception& e) { \
    error_dlg(e); \
    failure; \
  } \
  catch (...) { \
    far_message(L"\nFailure!", 0, FMSG_WARNING | FMSG_MB_OK); \
    failure; \
  }

extern ModuleVersion g_version;

void error_dlg(const Error& e);
void error_dlg(const std::exception& e);

UnicodeString get_temp_path();
