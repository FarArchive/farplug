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

#ifdef _INC_WINDOWS

inline AnsiString get_system_message(DWORD code) {
  AnsiString message;
  const wchar_t* sys_msg;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, code, 0, (LPWSTR) &sys_msg, 0, NULL);
  if (len != 0) {
    try {
      len = WideCharToMultiByte(CP_OEMCP, 0, sys_msg, len, message.get_buffer(len), len, NULL, NULL);
      if (len != 0) {
        message.release_buffer(len);
        message.strip_ctl();
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

class OleError: public Error {
private:
  HRESULT hr;
public:
  OleError(HRESULT hr): hr(hr) {
  }
  virtual AnsiString message() const {
    unsigned facility = (hr >> 16) & 0x7FF;
    unsigned code = hr & 0xFFFF;
    AnsiString facility_name;
    switch (facility) {
    case FACILITY_DISPATCH:
      facility_name = "FACILITY_DISPATCH";
      break;
    case FACILITY_ITF:
      facility_name = "FACILITY_ITF";
      break;
    case FACILITY_NULL:
      facility_name = "FACILITY_NULL";
      break;
    case FACILITY_RPC:
      facility_name = "FACILITY_RPC";
      break;
    case FACILITY_STORAGE:
      facility_name = "FACILITY_STORAGE";
      break;
    case FACILITY_WIN32:
      facility_name = "FACILITY_WIN32";
      break;
    case FACILITY_WINDOWS:
      facility_name = "FACILITY_WINDOWS";
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

#endif // _INC_WINDOWS

#endif /* _ERROR_H */
