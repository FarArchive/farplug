#pragma once

wstring get_system_message(HRESULT hr);
wstring get_console_title();
bool wait_for_single_object(HANDLE handle, DWORD timeout);
wstring ansi_to_unicode(const string& str, unsigned code_page);
string unicode_to_ansi(const wstring& str, unsigned code_page);
wstring expand_env_vars(const wstring& str);
wstring reg_query_value(HKEY h_key, const wchar_t* name, const wstring& def_value = wstring());
void reg_set_value(HKEY h_key, const wchar_t* name, const wstring& value);
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
  HANDLE handle() const;
  unsigned __int64 size();
  unsigned read(Buffer<char>& buffer);
  void write(const void* data, unsigned size);
};

class FindFile: private NonCopyable {
protected:
  wstring file_path;
  HANDLE h_find;
public:
  FindFile(const wstring& file_path);
  ~FindFile();
  bool next(WIN32_FIND_DATAW& find_data);
};

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
  Icon(WORD icon_id, int width, int height);
  ~Icon();
};
