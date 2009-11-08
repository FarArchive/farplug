#include <windows.h>

#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

#include "utils.hpp"
#include "sysutils.hpp"

FindFile::FindFile(const wstring& find_mask): h_find(INVALID_HANDLE_VALUE), find_mask(find_mask) {
}

FindFile::~FindFile() {
  if (h_find != INVALID_HANDLE_VALUE)
    FindClose(h_find);
}

bool FindFile::next(WIN32_FIND_DATAW& find_data) {
  if (h_find == INVALID_HANDLE_VALUE) {
    h_find = FindFirstFileW(long_path(find_mask).c_str(), &find_data);
    CHECK_SYS(h_find != INVALID_HANDLE_VALUE);
  }
  else {
    if (FindNextFileW(h_find, &find_data) == 0) {
      CHECK_SYS(GetLastError() == ERROR_NO_MORE_FILES);
      return false;
    }
  }
  return true;
}

wstring get_system_message(HRESULT hr) {
  wostringstream st;
  wchar_t* sys_msg;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&sys_msg), 0, NULL);
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
    st << strip(message) << L" (0x" << hex << setw(8) << setfill(L'0') << hr << L")";
  }
  else {
    st << L"HRESULT: 0x" << hex << setw(8) << setfill(L'0') << hr;
  }
  return st.str();
}
