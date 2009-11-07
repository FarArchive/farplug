#pragma once

struct Error {
  int hr;
  const char* file;
  int line;
};

#define FAIL(code) { \
  Error error; \
  error.hr = code; \
  error.file = __FILE__; \
  error.line = __LINE__; \
  throw error; \
}

#define CHECK_SYS(code) { if (!(code)) FAIL(HRESULT_FROM_WIN32(GetLastError())); }
#define CHECK_ADVSYS(code) { DWORD __ret = (code); if (__ret != ERROR_SUCCESS) FAIL(HRESULT_FROM_WIN32(SystemError(__ret))); }
