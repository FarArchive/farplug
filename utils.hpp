#pragma once

#define CHECK_SYS(code) { if (!(code)) throw HRESULT_FROM_WIN32(GetLastError()); }

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

bool substr_match(const wstring& str, wstring::size_type pos, wstring::const_pointer mstr);

wstring long_path(const wstring& path);

wstring add_trailing_slash(const wstring& path);
wstring del_trailing_slash(const wstring& path);

wstring extract_path_root(const wstring& path);
wstring extract_file_name(const wstring& path);
wstring extract_file_path(const wstring& path);
bool is_root_path(const wstring& path);
bool is_unc_path(const wstring& path);
bool is_absolute_path(const wstring& path);
