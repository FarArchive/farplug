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

class File: private NonCopyable {
private:
  HANDLE h_file;
public:
  File(const wstring& file_path, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes);
  ~File();
  HANDLE handle() const;
  unsigned __int64 size();
  unsigned read(Buffer<char>& buffer);
  void write(const char* data, unsigned size);
};

class FindFile: private NonCopyable {
private:
  wstring file_path;
  HANDLE h_find;
public:
  FindFile(const wstring& file_path);
  ~FindFile();
  bool next(WIN32_FIND_DATAW& find_data);
};

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

class CleanHandle: private NonCopyable {
private:
  HANDLE h;
public:
  CleanHandle(HANDLE h): h(h) {
  }
  ~CleanHandle() {
    CloseHandle(h);
  }
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
