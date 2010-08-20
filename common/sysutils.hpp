#pragma once

#ifdef DEBUG
  #define DEBUG_OUTPUT(msg) OutputDebugStringW(((msg) + L"\n").c_str())
#else
  #define DEBUG_OUTPUT(msg)
#endif

extern HINSTANCE g_h_instance;

wstring get_system_message(HRESULT hr);
wstring get_console_title();
bool wait_for_single_object(HANDLE handle, DWORD timeout);
wstring ansi_to_unicode(const string& str, unsigned code_page);
string unicode_to_ansi(const wstring& str, unsigned code_page);
wstring expand_env_vars(const wstring& str);
wstring get_full_path_name(const wstring& path);
wstring get_current_directory();

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

class File: private NonCopyable {
protected:
  HANDLE h_file;
public:
  File(const wstring& file_path, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes);
  ~File();
  unsigned __int64 size();
  unsigned read(Buffer<char>& buffer);
  void write(const void* data, unsigned size);
  void set_time(const FILETIME* ctime, const FILETIME* atime, const FILETIME* mtime);
};

class Key: private NonCopyable {
private:
  HKEY h_key;
  void close();
public:
  Key();
  ~Key();
  Key& create(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired);
  Key& open(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired);
  bool query_bool(const wchar_t* name, bool def_value = false);
  unsigned query_int(const wchar_t* name, unsigned def_value = 0);
  wstring query_str(const wchar_t* name, const wstring& def_value = wstring());
  void set_bool(const wchar_t* name, bool value);
  void set_int(const wchar_t* name, unsigned value);
  void set_str(const wchar_t* name, const wstring& value);
  void delete_value(const wchar_t* name);
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
  wstring dir_path;
  HANDLE h_find;
  FindData find_data;
public:
  FileEnum(const wstring& dir_path);
  ~FileEnum();
  bool next();
  const FindData& data() const {
    return find_data;
  }
};

FindData get_find_data(const wstring& path);

class TempFile: private NonCopyable {
private:
  wstring path;
public:
  TempFile();
  ~TempFile();
  wstring get_path() const {
    return path;
  }
};

class Thread: private NonCopyable {
private:
  Error error;
  static unsigned __stdcall thread_proc(void* arg);
protected:
  HANDLE h_thread;
  virtual void run() = 0;
public:
  Thread();
  virtual ~Thread();
  void start();
  bool wait(unsigned wait_time);
  bool get_result();
  Error get_error() {
    return error;
  }
};

class Event: private NonCopyable {
protected:
  HANDLE h_event;
public:
  Event(bool manual_reset, bool initial_state);
  ~Event();
  void set();
};

typedef LRESULT (CALLBACK *WindowProc)(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);

class WindowClass: private NonCopyable {
protected:
  wstring name;
public:
  WindowClass(const wstring& name, WindowProc window_proc);
  virtual ~WindowClass();
};

class MessageWindow: public WindowClass {
private:
  static LRESULT CALLBACK message_window_proc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);
protected:
  HWND h_wnd;
  virtual LRESULT window_proc(UINT msg, WPARAM w_param, LPARAM l_param) = 0;
  void end_message_loop(unsigned result);
public:
  MessageWindow(const wstring& name);
  virtual ~MessageWindow();
  unsigned message_loop(HANDLE h_abort);
};

class Icon: private NonCopyable {
protected:
  HICON h_icon;
public:
  Icon(HMODULE h_module, WORD icon_id, int width, int height);
  ~Icon();
};

wstring format_file_time(const FILETIME& file_time);
unsigned __int64 get_module_version(const wstring& file_path);
