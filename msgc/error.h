#ifndef _ERROR_H
#define _ERROR_H

#define finally(code) catch(...) { code; throw; } code;

#define FAIL(err) { \
  try { \
    throw err; \
  } \
  catch (Error& e) { \
    e.file = __FILE__; \
    e.line = __LINE__; \
    throw; \
  } \
}

#define NOFAIL(code) try { code; } catch (...) { }

#define CHECK(code, msg) { if (!(code)) FAIL(MsgError(msg)); }
#define CHECK_STD(code) { if (!(code)) FAIL(StdIoError()); }
#define CHECK_API(code) { if (!(code)) FAIL(SystemError()); }
#define CHECK_ADVAPI(code) { DWORD __ret; if ((__ret = (code)) != ERROR_SUCCESS) FAIL(SystemError(__ret)); }
#define CHECK_COM(code) { HRESULT __ret; if (FAILED(__ret = (code))) FAIL(ComError(__ret)); }

#define VERIFY(code) { if (!(code)) assert(false); }

class Error {
public:
  const char* file;
  unsigned line;
  virtual AnsiString message() const = 0;
};

class MsgError: public Error {
private:
  AnsiString msg;
public:
  MsgError(const AnsiString& msg): msg(msg) {
  }
  MsgError(const char* msg): msg(msg) {
  }
  virtual AnsiString message() const {
    return msg;
  }
};

#ifdef _ERROR_STDIO

class StdIoError: public Error {
private:
  int code;
public:
  StdIoError(): code(errno) {
  }
  virtual AnsiString message() const {
    return strerror(code);
  }
};

#endif // _ERROR_STDIO

#ifdef _ERROR_WINDOWS

inline AnsiString get_system_message(DWORD code) {
  AnsiString message;
  const wchar_t* sys_msg;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, code, 0, (LPWSTR) &sys_msg, 0, NULL);
  if (len != 0) {
    try {
      len = WideCharToMultiByte(CP_OEMCP, 0, sys_msg, len, message.buf(len), len, NULL, NULL);
      if (len != 0) {
        message.set_size(len);
        message.strip();
        message.add_fmt(" (%u)", code);
      }
    }
    finally (LocalFree((HLOCAL) sys_msg));
  }
  return message;
}

class SystemError: public Error {
private:
  DWORD code;
public:
  SystemError(): code(GetLastError()) {
  }
  SystemError(DWORD code): code(code) {
  }
  virtual AnsiString message() const {
    return get_system_message(code);
  }
};

class ComError: public Error {
private:
  HRESULT hr;
public:
  ComError(HRESULT hr): hr(hr) {
  }
  virtual AnsiString message() const {
    unsigned facility = (hr >> 16) & 0x7FF;
    unsigned code = hr & 0xFFFF;
    AnsiString facility_name;
    switch (facility) {
    case FACILITY_DISPATCH:
      facility_name = "DISPATCH";
      break;
    case FACILITY_ITF:
      facility_name = "ITF";
      break;
    case FACILITY_NULL:
      facility_name = "NULL";
      break;
    case FACILITY_RPC:
      facility_name = "RPC";
      break;
    case FACILITY_STORAGE:
      facility_name = "STORAGE";
      break;
    case FACILITY_WIN32:
      facility_name = "WIN32";
      break;
    case FACILITY_WINDOWS:
      facility_name = "WINDOWS";
      break;
    default:
      facility_name.copy_fmt("%u", facility);
      break;
    }
    if (facility == FACILITY_WIN32) {
      return AnsiString::format("COM error: 0x%x (facility = %S): %S", hr, &facility_name, &get_system_message(code));
    }
    else {
      return AnsiString::format("COM error: 0x%x (facility = %S, code = %u)", hr, &facility_name, code);
    }
  }
};

#ifdef MMRESULT

class MMError: public Error {
private:
  MMRESULT code;
public:
  MMError(MMRESULT code): code(code) {
  }
  virtual AnsiString message() const {
    return AnsiString::format("MMSYSTEM error: %u", code);
  }
};
#endif // MMRESULT

#endif // _ERROR_WINDOWS

#endif // _ERROR_H
