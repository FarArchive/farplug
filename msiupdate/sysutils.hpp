#pragma once

wstring get_system_message(HRESULT hr);
wstring get_console_title();
bool wait_for_single_object(HANDLE handle, DWORD timeout);
wstring ansi_to_unicode(const string& str, unsigned code_page);

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
