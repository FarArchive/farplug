#include <windows.h>

#include <stdio.h>
#include <math.h>

#include "plugin.hpp"

#include "col/AnsiString.h"
#include "col/UnicodeString.h"
#include "col/PlainArray.h"
#include "col/ObjectArray.h"
using namespace col;

#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "msg.h"

#include "dlgapi.h"
#include "options.h"
#include "log.h"
#include "utils.h"

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

// format amount of information
UnicodeString format_inf_amount(u64 size) {
  UnicodeString str1, str2;
  str1.copy_fmt(L"%Lu", size);
  for (unsigned i = 0; i < str1.size(); i++) {
    if (((str1.size() - i) % 3 == 0) && (i != 0)) {
      str2 += ',';
    }
    str2 += str1[i];
  }
  return str2;
}

UnicodeString format_inf_amount_short(u64 size, bool speed) {
  return format_data_size(size, speed ? speed_suffixes : size_suffixes);
}

ObjectArray<UnicodeString> size_suffixes;
ObjectArray<UnicodeString> speed_suffixes;
ObjectArray<UnicodeString> short_size_suffixes;

void load_suffixes() {
  size_suffixes.clear();
  size_suffixes += UnicodeString();
  size_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SIZE_KB : MSG_SUFFIX_SIZE_KB);
  size_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SIZE_MB : MSG_SUFFIX_SIZE_MB);
  size_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SIZE_GB : MSG_SUFFIX_SIZE_GB);
  size_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SIZE_TB : MSG_SUFFIX_SIZE_TB);
  speed_suffixes.clear();
  speed_suffixes += far_get_msg(MSG_SUFFIX_SPEED_B);
  speed_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SPEED_KB : MSG_SUFFIX_SPEED_KB);
  speed_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SPEED_MB : MSG_SUFFIX_SPEED_MB);
  speed_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SPEED_GB : MSG_SUFFIX_SPEED_GB);
  speed_suffixes += far_get_msg(g_use_standard_inf_units ? MSG_SUFFIX_ALT_SPEED_TB : MSG_SUFFIX_SPEED_TB);
  short_size_suffixes.clear();
  short_size_suffixes += UnicodeString(L" ");
  short_size_suffixes += far_get_msg(MSG_SUFFIX_SHORT_SIZE_KB);
  short_size_suffixes += far_get_msg(MSG_SUFFIX_SHORT_SIZE_MB);
  short_size_suffixes += far_get_msg(MSG_SUFFIX_SHORT_SIZE_GB);
  short_size_suffixes += far_get_msg(MSG_SUFFIX_SHORT_SIZE_TB);
}

UnicodeString format_data_size(unsigned __int64 value, const ObjectArray<UnicodeString>& suffixes) {
  unsigned f = 0;
  unsigned __int64 div = 1;
  while ((value / div >= 1000) && (f < 4)) {
    f++;
    div *= 1024;
  }
  unsigned __int64 v1 = value / div;

  unsigned __int64 mul;
  if (v1 < 10) mul = 100;
  else if (v1 < 100) mul = 10;
  else mul = 1;

  unsigned __int64 v2 = value % div;
  unsigned __int64 d = v2 * mul * 10 / div % 10;
  v2 = v2 * mul / div;
  if (d >= 5) {
    if (v2 + 1 == mul) {
      v2 = 0;
      if ((v1 == 999) && (f < 4)) {
        v1 = 0;
        v2 = 98;
        f += 1;
      }
      else v1 += 1;
    }
    else v2 += 1;
  }

  UnicodeString result;
  wchar_t buf[30];
  result.add(_ui64tow(v1, buf, 10));
  if (v2 != 0) {
    result += L'.';
    if ((v1 < 10) && (v2 < 10)) result += L'0';
    result.add(_ui64tow(v2, buf, 10));
  }
  if (suffixes[f].size() != 0) {
    result += L' ';
    result += suffixes[f];
  }
  return result;
}

UnicodeString format_time(u64 t /* ms */) {
  u64 ms = t % 1000;
  u64 s = (t / 1000) % 60;
  u64 m = (t / 1000 / 60) % 60;
  u64 h = t / 1000 / 60 / 60;
  return UnicodeString::format(L"%02Lu:%02Lu:%02Lu", h, m, s);
}

UnicodeString format_time2(u64 t /* ms */) {
  u64 ms = t % 1000;
  u64 s = (t / 1000) % 60;
  u64 m = (t / 1000 / 60) % 60;
  u64 h = t / 1000 / 60 / 60;
  if (h != 0) {
    if (m != 0) return UnicodeString::format(L"%Lu h %Lu m", h, m);
    else return UnicodeString::format(L"%Lu h", h);
  }
  else if (m != 0) {
    if (s != 0) return UnicodeString::format(L"%Lu m %Lu s", m, s);
    else return UnicodeString::format(L"%Lu m", m);
  }
  else if (s != 0) {
    if (ms != 0) return UnicodeString::format(L"%Lu.%03Lu s", s, ms);
    else return UnicodeString::format(L"%Lu s", s);
  }
  else return UnicodeString::format(L"%Lu ms", ms);
}

UnicodeString format_hex_array(const Array<u8>& a) {
  UnicodeString str;
  for (unsigned i = 0; i < a.size(); i++) {
    str.add_fmt(L"%Bx", a[i]);
  }
  return str;
}

// check if escape key is pressed
bool check_for_esc(void) {
  bool res = false;
  HANDLE h_con = GetStdHandle(STD_INPUT_HANDLE);
  INPUT_RECORD rec;
  DWORD read_cnt;
  while (true) {
    PeekConsoleInput(h_con, &rec, 1, &read_cnt);
    if (read_cnt == 0) break;
    ReadConsoleInput(h_con, &rec, 1, &read_cnt);
    if ((rec.EventType == KEY_EVENT) &&
      (rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) &&
      rec.Event.KeyEvent.bKeyDown &&
      ((rec.Event.KeyEvent.dwControlKeyState &
      (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED | SHIFT_PRESSED)) == 0)
    ) res = true;
  }
  return res;
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

// extract path root component (drive letter / volume name / UNC share)
UnicodeString extract_path_root(const UnicodeString& path) {
  unsigned prefix_len = 0;
  bool is_unc_path = false;
  if (path.equal(0, L"\\\\?\\UNC\\")) {
    prefix_len = 8;
    is_unc_path = true;
  }
  else if (path.equal(0, L"\\\\?\\") || path.equal(0, L"\\??\\") || path.equal(0, L"\\\\.\\")) {
    prefix_len = 4;
  }
  else if (path.equal(0, L"\\\\")) {
    prefix_len = 2;
    is_unc_path = true;
  }
  if ((prefix_len == 0) && !path.equal(1, L':')) return UnicodeString();
  unsigned p = path.search(prefix_len, L'\\');
  if (p == -1) p = path.size();
  if (is_unc_path) {
    p = path.search(p + 1, L'\\');
    if (p == -1) p = path.size();
  }
  return path.left(p);
}

UnicodeString extract_file_name(const UnicodeString& path) {
  unsigned pos = path.rsearch('\\');
  if (pos == -1) pos = 0;
  else pos++;
  if (pos < extract_path_root(path).size()) return UnicodeString();
  return path.slice(pos);
}

UnicodeString extract_file_dir(const UnicodeString& path) {
  unsigned pos = path.rsearch('\\');
  if (pos == -1) pos = 0;
  if (pos < extract_path_root(path).size()) return UnicodeString();
  return path.left(pos);
}

UnicodeString long_path(const UnicodeString& path) {
  if (path.size() < MAX_PATH) return path;
  if (path.equal(0, L"\\\\?\\") || path.equal(0, L"\\\\.\\")) return path;
  if (path.equal(0, L"\\??\\")) return UnicodeString(path).replace(0, 4, L"\\\\?\\");
  if (path.equal(0, L"\\\\")) return UnicodeString(path).replace(0, 1, L"\\\\?\\UNC");
  return L"\\\\?\\" + path;
}

UnicodeString add_trailing_slash(const UnicodeString& file_path) {
  if ((file_path.size() == 0) || (file_path.last() == L'\\')) return file_path;
  else return file_path + L'\\';
}

UnicodeString del_trailing_slash(const UnicodeString& file_path) {
  if ((file_path.size() < 2) || (file_path.last() != L'\\')) return file_path;
  else return file_path.left(file_path.size() - 1);
}

int str_to_int(const UnicodeString& str) {
  return _wtoi(str.data());
}

UnicodeString int_to_str(int val) {
  wchar_t str[64];
  return _itow(val, str, 10);
}

UnicodeString center(const UnicodeString& str, unsigned width) {
  if (str.size() >= width) return str;
  unsigned lpad = (width - str.size()) / 2;
  unsigned rpad = width - lpad;
  return UnicodeString::format(L"%.*c%S%.*c", lpad, ' ', &str, rpad, ' ');
}

UnicodeString fit_str(const UnicodeString& path, unsigned size) {
  if (path.size() <= size) return path;
  size -= 3; // place for ...
  unsigned ls = size / 2; // left part size
  unsigned rs = size - ls; // right part size
  return path.left(ls) + L"..." + path.right(rs);
}

void unquote(UnicodeString& str) {
  if ((str.size() >= 2) && (str[0] == '"') && (str.last() == '"')) {
    str.remove(0);
    str.remove(str.size() - 1);
  }
}

ObjectArray<UnicodeString> split_str(const UnicodeString& str, wchar_t split_ch) {
  ObjectArray<UnicodeString> list;
  unsigned pos = 0;
  while (pos < str.size()) {
    unsigned pos2 = str.search(pos, split_ch);
    if (pos2 == -1) pos2 = str.size();
    list += str.slice(pos, pos2 - pos);
    pos = pos2 + 1;
  }
  return list;
}

ProgressMonitor::ProgressMonitor(bool lazy): h_scr(NULL) {
  unsigned __int64 t_curr;
  QueryPerformanceCounter((PLARGE_INTEGER) &t_curr);
  QueryPerformanceFrequency((PLARGE_INTEGER) &t_freq);
  if (lazy) t_next = t_curr + t_freq / 2;
  else t_next = t_curr;
}

ProgressMonitor::~ProgressMonitor() {
  if (h_scr != NULL) {
    g_far.RestoreScreen(h_scr);
    SetConsoleTitleW(con_title.data());
  }
}

void ProgressMonitor::update_ui(bool force) {
  unsigned __int64 t_curr;
  QueryPerformanceCounter((PLARGE_INTEGER) &t_curr);
  if ((t_curr >= t_next) || force) {
    if (h_scr == NULL) {
      g_far.Text(0, 0, 0, NULL); // flush buffer hack
      h_scr = g_far.SaveScreen(0, 0, -1, -1);
      unsigned c_max_size = 10000;
      con_title.set_size(GetConsoleTitleW(con_title.buf(c_max_size), c_max_size)).compact();
    }
    HANDLE h_con = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD rec;
    DWORD read_cnt;
    while (true) {
      PeekConsoleInput(h_con, &rec, 1, &read_cnt);
      if (read_cnt == 0) break;
      ReadConsoleInput(h_con, &rec, 1, &read_cnt);
      if ((rec.EventType == KEY_EVENT) && (rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) && rec.Event.KeyEvent.bKeyDown && ((rec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED | SHIFT_PRESSED)) == 0)) BREAK;
    }
    t_next = t_curr + t_freq / 2;
    do_update_ui();
  }
}

void Log::show() {
  try {
    // create file name for temporary file
    UnicodeString temp_file_path;
    CHECK_SYS(GetTempFileNameW(get_temp_path().data(), L"log", 0, temp_file_path.buf(MAX_PATH)));
    temp_file_path.set_size();

    // schedule file deletion
    CLEAN(const UnicodeString&, temp_file_path, DeleteFileW(temp_file_path.data()));
    {
      // open file for writing
      HANDLE h_file = CreateFileW(temp_file_path.data(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
      CLEAN(HANDLE, h_file, CHECK_SYS(CloseHandle(h_file)));

      // write unicode signature
      const wchar_t sig = 0xFEFF;
      DWORD size_written;
      CHECK_SYS(WriteFile(h_file, &sig, sizeof(sig), &size_written, NULL));
      // write log data
      UnicodeString line;
      for (unsigned i = 0; i < size(); i++) {
        UnicodeString op;
        if (citem(i).type == lotDefragment) op = far_get_msg(MSG_LOG_DEFRAGMENT);
        line.copy_fmt(L"%S: '%S' (%S)\n", &op, &citem(i).object, &citem(i).message);
        CHECK_SYS(WriteFile(h_file, line.data(), line.size() * sizeof(wchar_t), &size_written, NULL));
      }
    }

    // open temporary file in Far viewer
    far_viewer(temp_file_path, far_get_msg(MSG_LOG_TITLE));
  }
  catch (...) {
  }
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

FarStr far_get_cur_dir(HANDLE h_panel, const PanelInfo& pi) {
  FarStr cur_dir;
#ifdef FARAPI17
  return pi.CurDir;
#endif
#ifdef FARAPI18
  unsigned cur_dir_size = g_far.Control(h_panel, FCTL_GETCURRENTDIRECTORY, 0, 0) - 1;
  g_far.Control(h_panel, FCTL_GETCURRENTDIRECTORY, cur_dir_size + 1, reinterpret_cast<LONG_PTR>(cur_dir.buf(cur_dir_size)));
  cur_dir.set_size(cur_dir_size);
#endif
  return cur_dir;
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

#ifdef FARAPI17
#  ifdef _WIN64
#    define PLUGIN_TYPE L"x64"
#  else
#    define PLUGIN_TYPE L""
#  endif
#endif
#ifdef FARAPI18
#  ifdef _WIN64
#    define PLUGIN_TYPE L"uni x64"
#  else
#    define PLUGIN_TYPE L"uni"
#  endif
#endif

ModuleVersion g_version;

void error_dlg(const Error& e) {
  UnicodeString msg;
  msg.add(far_get_msg(MSG_PLUGIN_NAME)).add('\n');
  UnicodeString err_msg = word_wrap(e.message(), get_msg_width());
  if (err_msg.size() != 0) msg.add(err_msg).add('\n');
  msg.add_fmt(L"%S:%u r.%u "PLUGIN_TYPE, &extract_file_name(oem_to_unicode(e.file)), e.line, g_version.revision);
  far_message(msg, 0, FMSG_WARNING | FMSG_MB_OK);
}

void error_dlg(const std::exception& e) {
  UnicodeString msg;
  msg.add(far_get_msg(MSG_PLUGIN_NAME)).add('\n');
  UnicodeString err_msg = word_wrap(oem_to_unicode(e.what()), get_msg_width());
  if (err_msg.size() != 0) msg.add(err_msg).add('\n');
  msg.add_fmt(L"r.%u "PLUGIN_TYPE, g_version.revision);
  far_message(msg, 0, FMSG_WARNING | FMSG_MB_OK);
}

// get system directory for temporary files
UnicodeString get_temp_path() {
  UnicodeString temp_path;
  DWORD temp_path_size = MAX_PATH;
  DWORD len = GetTempPathW(temp_path_size, temp_path.buf(temp_path_size));
  if (len > temp_path_size) {
    temp_path_size = len;
    len = GetTempPathW(temp_path_size, temp_path.buf(temp_path_size));
  }
  CHECK_SYS(len != 0);
  temp_path.set_size(len);
  return temp_path;
}
