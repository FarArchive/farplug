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
#define CHECK_COM(code) { HRESULT __ret = (code); if (FAILED(__ret)) FAIL(__ret); }

#define CLEAN(type, object, code) \
  class Clean_##object { \
  private: \
    type object; \
  public: \
    Clean_##object(type object): object(object) { \
    } \
    ~Clean_##object() { \
      code; \
    } \
  }; \
  Clean_##object clean_##object(object);
