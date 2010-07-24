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

class NonCopyable {
protected:
  NonCopyable() {}
  ~NonCopyable() {}
private:
  NonCopyable(const NonCopyable&);
  NonCopyable& operator=(const NonCopyable&);
};

void unicode_to_oem(AnsiString& oem_str, const UnicodeString& u_str);
AnsiString unicode_to_oem(const UnicodeString& str);
void oem_to_unicode(UnicodeString& u_str, const AnsiString& oem_str);
UnicodeString oem_to_unicode(const AnsiString& str);
AnsiString unicode_to_ansi(const UnicodeString& u_str);
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

class CriticalSection: private NonCopyable, private CRITICAL_SECTION {
public:
  CriticalSection() {
    InitializeCriticalSection(this);
  }
  virtual ~CriticalSection() {
    DeleteCriticalSection(this);
  }
  friend class CriticalSectionLock;
};

class CriticalSectionLock: private NonCopyable {
private:
  CriticalSection& cs;
public:
  CriticalSectionLock(CriticalSection& cs): cs(cs) {
    EnterCriticalSection(&cs);
  }
  ~CriticalSectionLock() {
    LeaveCriticalSection(&cs);
  }
};

class Event: private NonCopyable {
protected:
  HANDLE h_event;
public:
  Event(bool manual_reset, bool initial_state);
  ~Event();
  HANDLE handle() const {
    return h_event;
  }
};

class Semaphore: private NonCopyable {
protected:
  HANDLE h_sem;
public:
  Semaphore(unsigned init_cnt, unsigned max_cnt);
  ~Semaphore();
  HANDLE handle() const {
    return h_sem;
  }
};

class File: private NonCopyable {
protected:
  HANDLE h_file;
public:
  File(const UnicodeString& file_path, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes);
  File(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTransaction);
  ~File();
  HANDLE handle() const {
    return h_file;
  }
  unsigned __int64 pos();
  unsigned __int64 size();
  unsigned read(void* data, unsigned size);
  void write(const void* data, unsigned size);
};

struct FindData: public WIN32_FIND_DATAW {
  bool is_dir() const {
    return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  unsigned __int64 size() const {
    return (static_cast<unsigned __int64>(nFileSizeHigh) << 32) | nFileSizeLow;
  }
};

class FileEnum: private NonCopyable {
protected:
  UnicodeString dir_path;
  HANDLE h_find;
  FindData find_data;
public:
  FileEnum(const UnicodeString& dir_path);
  ~FileEnum();
  bool next();
  const FindData& data() const {
    return find_data;
  }
};

FindData get_find_data(const UnicodeString& path);
