#pragma once

struct Error {
  int code;
  wstring message;
  const char* file;
  int line;
};

#define FAIL(_code) { \
  Error error; \
  error.code = _code; \
  error.file = __FILE__; \
  error.line = __LINE__; \
  throw error; \
}

#define FAIL_MSG(_message) { \
  Error error; \
  error.code = NO_ERROR; \
  error.message = _message; \
  error.file = __FILE__; \
  error.line = __LINE__; \
  throw error; \
}

#define CHECK_SYS(code) { if (!(code)) FAIL(HRESULT_FROM_WIN32(GetLastError())); }
#define CHECK_ADVSYS(code) { DWORD __ret = (code); if (__ret != ERROR_SUCCESS) FAIL(HRESULT_FROM_WIN32(__ret)); }
