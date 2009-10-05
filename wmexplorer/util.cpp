#include <windows.h>

#include "plugin.hpp"

#include <stdio.h>
#include <math.h>

#include "col/AnsiString.h"
#include "col/UnicodeString.h"
#include "col/PlainArray.h"
#include "col/ObjectArray.h"
using namespace col;

#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "util.h"

extern struct PluginStartupInfo g_far;

// unicode <-> oem codepage conversions
void unicode_to_oem(AnsiString& oem_str, const UnicodeString& u_str) {
  unsigned size = u_str.size() + 1;
  int res = WideCharToMultiByte(CP_OEMCP, 0, u_str.data(), size, oem_str.buf(u_str.size()), size, NULL, NULL);
  if (res == 0) FAIL(SystemError());
  oem_str.set_size(res - 1);
}

void oem_to_unicode(UnicodeString& u_str, const AnsiString& oem_str) {
  unsigned size = oem_str.size() + 1;
  int res = MultiByteToWideChar(CP_OEMCP, 0, oem_str.data(), size, u_str.buf(oem_str.size()), size);
  if (res == 0) FAIL(SystemError());
  u_str.set_size(res - 1);
}

AnsiString unicode_to_oem(const UnicodeString& u_str) {
  AnsiString oem_str;
  unicode_to_oem(oem_str, u_str);
  return oem_str;
}

UnicodeString oem_to_unicode(const AnsiString& oem_str) {
  UnicodeString u_str;
  oem_to_unicode(u_str, oem_str);
  return u_str;
}

void unicode_to_ansi(AnsiString& a_str, const UnicodeString& u_str) {
  unsigned size = u_str.size() + 1;
  int res = WideCharToMultiByte(CP_ACP, 0, u_str.data(), size, a_str.buf(u_str.size()), size, NULL, NULL);
  if (res == 0) FAIL(SystemError());
  a_str.set_size(res - 1);
}

void ansi_to_unicode(UnicodeString& u_str, const AnsiString& a_str) {
  unsigned size = a_str.size() + 1;
  int res = MultiByteToWideChar(CP_ACP, 0, a_str.data(), size, u_str.buf(a_str.size()), size);
  if (res == 0) FAIL(SystemError());
  u_str.set_size(res - 1);
}

AnsiString unicode_to_ansi(const UnicodeString& u_str) {
  AnsiString a_str;
  unicode_to_ansi(a_str, u_str);
  return a_str;
}

UnicodeString ansi_to_unicode(const AnsiString& a_str) {
  UnicodeString u_str;
  ansi_to_unicode(u_str, a_str);
  return u_str;
}

UnicodeString word_wrap(const UnicodeString& message, unsigned wrap_bound) {
  UnicodeString msg = message;
  unsigned limit = wrap_bound;
  unsigned idx = -1;
  for (unsigned i = 0; i < msg.size(); i++) {
    if (i >= limit) {
      if (idx != -1) {
        msg.insert(idx, '\n');
        i = idx + 1;
        limit = idx + 2 + wrap_bound;
        idx = -1;
        continue;
      }
    }
    if (msg[i] == ' ') idx = i;
  }
  return msg;
}

UnicodeString format_data_size(unsigned __int64 size, const ObjectArray<UnicodeString>& suffixes) {
  unsigned f = 0;
  double v = (double) size;
  while ((v >= 1024) && (f < 4)) {
    f++;
    v /= 1024;
  }

  double mul;
  if (v < 10) mul = 100;
  else if (v < 100) mul = 10;
  else mul = 1;

  v *= mul;
  if (v - floor(v) >= 0.5) v += 1;
  v = floor(v) / mul;
  if ((v == 1024) && (f < 4)) {
    v = 1;
    f++;
  }

  wchar_t buf[16];
  swprintf(buf, L"%g %s", v, suffixes[f].data());
  return UnicodeString(buf);
}

UnicodeString format_time(unsigned __int64 t /* ms */) {
  unsigned __int64 ms = t % 1000;
  unsigned __int64 s = (t / 1000) % 60;
  unsigned __int64 m = (t / 1000 / 60) % 60;
  unsigned __int64 h = t / 1000 / 60 / 60;
  return UnicodeString::format(L"%02Lu:%02Lu:%02Lu", h, m, s);
}

UnicodeString fit_str(const UnicodeString& path, unsigned size) {
  if (path.size() <= size) return path;
  size -= 3; // place for ...
  unsigned ls = size / 2; // left part size
  unsigned rs = size - ls; // right part size
  return path.left(ls) + L"..." + path.right(rs);
}

UnicodeString center(const UnicodeString& str, unsigned width) {
  if (str.size() >= width) return str;
  unsigned lpad = (width - str.size()) / 2;
  unsigned rpad = width - str.size() - lpad;
  return UnicodeString::format(L"%.*c%S%.*c", lpad, ' ', &str, rpad, ' ');
}

unsigned __int64 mul_div(unsigned __int64 a, unsigned __int64 b, unsigned __int64 c) {
  double r_double = (double) a * ((double) b / (double) c);
  assert(r_double < 0xFFFFFFFFFFFFFFFFl);
  unsigned __int64 r_int = (unsigned __int64) r_double;
  if (r_double - r_int >= 0.5) r_int++;
  return r_int;
}

ModuleVersion get_module_version(HINSTANCE module) {
  ModuleVersion version;
  memset(&version, 0, sizeof(version));
  UnicodeString file_name;
  unsigned size = GetModuleFileNameW(module, file_name.buf(MAX_PATH), MAX_PATH);
  if (size != 0) {
    file_name.set_size(size);
    DWORD handle;
    size = GetFileVersionInfoSizeW(file_name.data(), &handle);
    if (size != 0) {
      Array<char> ver_data;
      if (GetFileVersionInfoW(file_name.data(), handle, size, ver_data.buf(size)) != 0) {
        VS_FIXEDFILEINFO* ver;
        if (VerQueryValueW((const LPVOID) ver_data.data(), L"\\", (LPVOID*) &ver, &size) != 0) {
          version.major = HIWORD(ver->dwProductVersionMS);
          version.minor = LOWORD(ver->dwProductVersionMS);
          version.patch = HIWORD(ver->dwProductVersionLS);
          version.revision = LOWORD(ver->dwProductVersionLS);
        }
      }
    }
  }
  return version;
}

const UnicodeString extract_file_name(const UnicodeString& file_path) {
  unsigned pos = file_path.rsearch('\\');
  if (pos == -1) pos = 0;
  else pos++;
  return file_path.slice(pos, file_path.size() - pos);
}

void unquote(UnicodeString& str) {
  if ((str.size() >= 2) && (str[0] == '"') && (str.last() == '"')) {
    str.remove(0);
    str.remove(str.size() - 1);
  }
}

#ifdef FARAPI17
// special encoding for file names that cannot be converted into OEM codepage
void encode_fn(UnicodeString& dst, const UnicodeString& src) {
  AnsiString str;
  unicode_to_oem(str, src);
  oem_to_unicode(dst, str);
  for (unsigned i = 0, j = 0; (i < src.size()) && (j < dst.size()); i++, j++) {
    if (dst[j] != src[i]) {
      if (src[i] > 0xFF) {
        dst.replace_fmt(j, 1, L"**%.4x", src[i]);
        j += 5;
      }
      else {
        dst.replace_fmt(j, 1, L"*%.2x", src[i]);
        j += 2;
      }
    }
  }
}

UnicodeString encode_fn(const UnicodeString& src) {
  UnicodeString dst;
  encode_fn(dst, src);
  return dst;
}

void decode_fn(UnicodeString& dst, const UnicodeString& src) {
  dst.clear();
  dst.extend(src.size());
  unsigned i = 0;
  while (i < src.size()) {
    if (src[i] == L'*') {
      i++;
      if (i < src.size()) {
        unsigned l = 2;
        if (src[i] == L'*') {
          l = 4;
          i++;
        }
        if (i + l <= src.size()) {
          wchar_t ch = 0;
          for (unsigned j = i; j < i + l; j++) {
            if ((src[j] >= '0') && (src[j] <= '9')) {
              ch = ch * 16 + (src[j] - '0');
            }
            else if ((src[j] >= 'A') && (src[j] <= 'F')) {
              ch = ch * 16 + (src[j] - 'A' + 10);
            }
          }
          dst += ch;
          i += l;
        }
      }
    }
    else {
      dst += src[i];
      i++;
    }
  }
}

UnicodeString decode_fn(const UnicodeString& src) {
  UnicodeString dst;
  decode_fn(dst, src);
  return dst;
}
#endif // FARAPI17

UnicodeString make_temp_file() {
  UnicodeString temp_path;
  CHECK_API(GetTempPathW(MAX_PATH, temp_path.buf(MAX_PATH)) != 0);
  temp_path.set_size();
  UnicodeString temp_file_name;
  CHECK_API(GetTempFileNameW(temp_path.data(), L"wme", 0, temp_file_name.buf(MAX_PATH)) != 0);
  temp_file_name.set_size();
  return temp_file_name;
}

UnicodeString format_file_time(const FILETIME& file_time) {
  FILETIME local_ft;
  if (FileTimeToLocalFileTime(&file_time, &local_ft) == 0) return UnicodeString();
  SYSTEMTIME st;
  if (FileTimeToSystemTime(&local_ft, &st) == 0) return UnicodeString();
  const unsigned c_size = 1024;
  UnicodeString date_str;
  if (GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, date_str.buf(c_size), c_size) == 0) return UnicodeString();
  date_str.set_size();
  UnicodeString time_str;
  if (GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, time_str.buf(c_size), c_size) == 0) return UnicodeString();
  time_str.set_size();
  return date_str + L' ' + time_str;
}

UnicodeString add_trailing_slash(const UnicodeString& file_path) {
  if ((file_path.size() == 0) || (file_path.last() == L'\\')) return file_path;
  else return file_path + L'\\';
}

UnicodeString del_trailing_slash(const UnicodeString& file_path) {
  if ((file_path.size() < 2) || (file_path.last() != L'\\')) return file_path;
  else return file_path.left(file_path.size() - 1);
}

int far_control_int(HANDLE h_panel, int command, int param) {
#ifdef FARAPI17
  return g_far.Control(h_panel, command, reinterpret_cast<void*>(param));
#endif
#ifdef FARAPI18
  return g_far.Control(h_panel, command, param, 0);
#endif
}

int far_control_ptr(HANDLE h_panel, int command, const void* param) {
#ifdef FARAPI17
  return g_far.Control(h_panel, command, const_cast<void*>(param));
#endif
#ifdef FARAPI18
  return g_far.Control(h_panel, command, 0, reinterpret_cast<LONG_PTR>(param));
#endif
}

PluginPanelItem* far_get_panel_item(HANDLE h_panel, int index, const PanelInfo& pi) {
#ifdef FARAPI17
  return pi.PanelItems + index;
#endif
#ifdef FARAPI18
  static Array<unsigned char> ppi;
  unsigned size = g_far.Control(h_panel, FCTL_GETPANELITEM, index, NULL);
  ppi.extend(size);
  g_far.Control(h_panel, FCTL_GETPANELITEM, index, reinterpret_cast<LONG_PTR>(ppi.buf()));
  ppi.set_size(size);
  return reinterpret_cast<PluginPanelItem*>(ppi.buf());
#endif
}

PluginPanelItem* far_get_selected_panel_item(HANDLE h_panel, int index, const PanelInfo& pi) {
#ifdef FARAPI17
  return pi.SelectedItems + index;
#endif
#ifdef FARAPI18
  static Array<unsigned char> ppi;
  unsigned size = g_far.Control(h_panel, FCTL_GETSELECTEDPANELITEM, index, NULL);
  ppi.extend(size);
  g_far.Control(h_panel, FCTL_GETSELECTEDPANELITEM, index, reinterpret_cast<LONG_PTR>(ppi.buf()));
  ppi.set_size(size);
  return reinterpret_cast<PluginPanelItem*>(ppi.buf());
#endif
}

UnicodeString far_get_current_dir() {
  UnicodeString curr_dir;
#ifdef FARAPI17
  DWORD size = GetCurrentDirectoryW(0, NULL);
  CHECK_API(size != 0);
  CHECK_API(GetCurrentDirectoryW(size, curr_dir.buf(size)) != 0);
  curr_dir.set_size();
#endif
#ifdef FARAPI18
  DWORD size = g_far.Control(INVALID_HANDLE_VALUE, FCTL_GETCURRENTDIRECTORY, 0, NULL);
  g_far.Control(INVALID_HANDLE_VALUE, FCTL_GETCURRENTDIRECTORY, size, reinterpret_cast<LONG_PTR>(curr_dir.buf(size)));
  curr_dir.set_size();
#endif
  return del_trailing_slash(curr_dir);
}
