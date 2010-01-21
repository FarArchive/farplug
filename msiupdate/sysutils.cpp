#include "utils.hpp"
#include "sysutils.hpp"

wstring get_system_message(HRESULT hr) {
  wostringstream st;
  wchar_t* sys_msg;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&sys_msg), 0, NULL);
  if (!len) {
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32) {
      HMODULE h_winhttp = LoadLibrary("winhttp");
      len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, h_winhttp, HRESULT_CODE(hr), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&sys_msg), 0, NULL);
      FreeLibrary(h_winhttp);
    }
  }
  if (len) {
    wstring message;
    try {
      message = sys_msg;
    }
    catch (...) {
      LocalFree(static_cast<HLOCAL>(sys_msg));
      throw;
    }
    LocalFree(static_cast<HLOCAL>(sys_msg));
    st << strip(message) << L" (0x" << hex << uppercase << setw(8) << setfill(L'0') << hr << L")";
  }
  else {
    st << L"HRESULT: 0x" << hex << uppercase << setw(8) << setfill(L'0') << hr;
  }
  return st.str();
}

wstring get_console_title() {
  Buffer<wchar_t> buf(10000);
  DWORD size = GetConsoleTitleW(buf.data(), static_cast<DWORD>(buf.size()));
  return wstring(buf.data(), size);
}

bool wait_for_single_object(HANDLE handle, DWORD timeout) {
  DWORD res = WaitForSingleObject(handle, timeout);
  CHECK_SYS(res != WAIT_FAILED);
  if (res == WAIT_OBJECT_0)
    return true;
  else if (res == WAIT_TIMEOUT)
    return false;
  else
    FAIL(E_FAIL);
}

wstring ansi_to_unicode(const string& str, unsigned code_page) {
  unsigned str_size = static_cast<unsigned>(str.size());
  if (str_size == 0)
    return wstring();
  int size = MultiByteToWideChar(code_page, 0, str.data(), str_size, NULL, 0);
  Buffer<wchar_t> out(size);
  size = MultiByteToWideChar(code_page, 0, str.data(), str_size, out.data(), out.size());
  CHECK_SYS(size);
  return wstring(out.data(), size);
}

string unicode_to_ansi(const wstring& str, unsigned code_page) {
  unsigned str_size = static_cast<unsigned>(str.size());
  if (str_size == 0)
    return string();
  int size = WideCharToMultiByte(code_page, 0, str.data(), str_size, NULL, 0, NULL, NULL);
  Buffer<char> out(size);
  size = WideCharToMultiByte(code_page, 0, str.data(), str_size, out.data(), out.size(), NULL, NULL);
  CHECK_SYS(size);
  return string(out.data(), size);
}

wstring expand_env_vars(const wstring& str) {
  Buffer<wchar_t> buf(MAX_PATH);
  unsigned size = ExpandEnvironmentStringsW(str.c_str(), buf.data(), static_cast<DWORD>(buf.size()));
  if (size > buf.size()) {
    buf.resize(size);
    size = ExpandEnvironmentStringsW(str.c_str(), buf.data(), static_cast<DWORD>(buf.size()));
  }
  CHECK_SYS(size);
  return wstring(buf.data(), size - 1);
}

wstring reg_query_value(HKEY h_key, const wchar_t* name, const wstring& def_value) {
  DWORD type = REG_SZ;
  DWORD data_size;
  LONG res = RegQueryValueExW(h_key, name, NULL, &type, NULL, &data_size);
  if (res == ERROR_SUCCESS) {
    Buffer<wchar_t> buf(data_size / sizeof(wchar_t));
    res = RegQueryValueExW(h_key, name, NULL, &type, reinterpret_cast<LPBYTE>(buf.data()), &data_size);
    if (res == ERROR_SUCCESS) {
      return wstring(buf.data(), buf.size() - 1);
    }
  }
  return def_value;
}

void reg_set_value(HKEY h_key, const wchar_t* name, const wstring& value) {
  CHECK_ADVSYS(RegSetValueExW(h_key, name, 0, REG_SZ, reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(value.c_str())), (static_cast<DWORD>(value.size()) + 1) * sizeof(wchar_t)));
}

wstring get_full_path_name(const wstring& path) {
  Buffer<wchar_t> buf(MAX_PATH);
  DWORD size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.size()), buf.data(), NULL);
  if (size > buf.size()) {
    buf.resize(size);
    size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.size()), buf.data(), NULL);
  }
  CHECK_SYS(size);
  return wstring(buf.data(), size);
}

wstring get_current_directory() {
  Buffer<wchar_t> buf(MAX_PATH);
  DWORD size = GetCurrentDirectoryW(static_cast<DWORD>(buf.size()), buf.data());
  if (size > buf.size()) {
    buf.resize(size);
    size = GetCurrentDirectoryW(static_cast<DWORD>(buf.size()), buf.data());
  }
  CHECK_SYS(size);
  return wstring(buf.data(), size);
}

File::File(const wstring& file_path, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes) {
  h_file = CreateFileW(long_path(file_path).c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
  CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
}

File::~File() {
  CloseHandle(h_file);
}

HANDLE File::handle() const {
  return h_file;
}

unsigned __int64 File::size() {
  LARGE_INTEGER file_size;
  CHECK_SYS(GetFileSizeEx(h_file, &file_size));
  return file_size.QuadPart;
}

unsigned File::read(Buffer<char>& buffer) {
  DWORD size_read;
  CHECK_SYS(ReadFile(h_file, buffer.data(), static_cast<DWORD>(buffer.size()), &size_read, NULL));
  return size_read;
}

void File::write(const void* data, unsigned size) {
  DWORD size_written;
  CHECK_SYS(WriteFile(h_file, data, size, &size_written, NULL));
}

FindFile::FindFile(const wstring& file_path): file_path(file_path), h_find(INVALID_HANDLE_VALUE) {
}

FindFile::~FindFile() {
  if (h_find != INVALID_HANDLE_VALUE)
    FindClose(h_find);
}

bool FindFile::next(WIN32_FIND_DATAW& find_data) {
  while (true) {
    if (h_find == INVALID_HANDLE_VALUE) {
      h_find = FindFirstFileW(long_path(add_trailing_slash(file_path) + L'*').c_str(), &find_data);
      CHECK_SYS(h_find != INVALID_HANDLE_VALUE);
    }
    else {
      if (!FindNextFileW(h_find, &find_data)) {
        if (GetLastError() == ERROR_NO_MORE_FILES)
          return false;
        CHECK_SYS(false);
      }
    }
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if ((find_data.cFileName[0] == L'.') && ((find_data.cFileName[1] == 0) || ((find_data.cFileName[1] == L'.') && (find_data.cFileName[2] == 0))))
        continue;
    }
    return true;
  }
}

TempFile::TempFile() {
  Buffer<wchar_t> buf(MAX_PATH);
  DWORD len = GetTempPathW(static_cast<DWORD>(buf.size()), buf.data());
  CHECK(len <= buf.size());
  CHECK_SYS(len);
  wstring temp_path = wstring(buf.data(), len);
  CHECK_SYS(GetTempFileNameW(temp_path.c_str(), L"", 0, buf.data()));
  path.assign(buf.data());
}

TempFile::~TempFile() {
  DeleteFileW(path.c_str());
}

unsigned __stdcall Thread::thread_proc(void* arg) {
  Thread* thread = reinterpret_cast<Thread*>(arg);
  try {
    try {
      thread->run();
      return TRUE;
    }
    catch (const Error&) {
      throw;
    }
    catch (const std::exception& e) {
      FAIL_MSG(widen(e.what()));
    }
    catch (...) {
      FAIL(E_FAIL);
    }
  }
  catch (const Error& e) {
    thread->error = e;
  }
  catch (...) {
  }
  return FALSE;
}

Thread::Thread(): h_thread(NULL) {
}

Thread::~Thread() {
  if (h_thread) {
    wait(INFINITE);
    CloseHandle(h_thread);
  }
}

void Thread::start() {
  unsigned th_id;
  h_thread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, thread_proc, this, 0, &th_id));
  CHECK_SYS(h_thread);
}

bool Thread::wait(unsigned wait_time) {
  return wait_for_single_object(h_thread, wait_time);
}

bool Thread::get_result() {
  DWORD exit_code;
  CHECK_SYS(GetExitCodeThread(h_thread, &exit_code));
  return exit_code == TRUE ? true : false;
}

Event::Event(bool manual_reset, bool initial_state) {
  h_event = CreateEvent(NULL, manual_reset, initial_state, NULL);
  CHECK_SYS(h_event);
}

Event::~Event() {
  CloseHandle(h_event);
}

void Event::set() {
  CHECK_SYS(SetEvent(h_event));
}
