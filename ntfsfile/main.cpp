#include <windows.h>
#include <winioctl.h>

#include <process.h>
#include <time.h>

#include <list>

#include "plugin.hpp"
#include "farcolor.hpp"
#include "farkeys.hpp"

#include "col/AnsiString.h"
#include "col/UnicodeString.h"
#include "col/PlainArray.h"
#include "col/ObjectArray.h"
using namespace col;

#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "msg.h"

#include "utils.h"
#include "ntfs.h"
#include "volume.h"
#include "options.h"
#include "content.h"
#include "dlgapi.h"
#include "ntfs_file.h"
#include "file_panel.h"
#include "log.h"
#include "defragment.h"

struct PluginStartupInfo g_far;
struct FarStandardFunctions g_fsf;

Array<unsigned char> g_colors;

HINSTANCE g_h_module;

struct CtrlIds {
  unsigned linked;
  unsigned link; // displays text from 'linked'
  CtrlIds(): linked(-1) { // no linked control by default
  }
};

const int c_update_time = 1;

struct FileTotals {
  u64 unnamed_data_size;
  u64 named_data_size;
  u64 nr_data_size;
  u64 unnamed_disk_size;
  u64 named_disk_size;
  u64 nr_disk_size;
  u64 unnamed_hl_size;
  u64 named_hl_size;
  u64 nr_hl_size;
  u64 excess_fragments;
  unsigned file_cnt;
  unsigned hl_cnt;
  unsigned dir_cnt;
  unsigned file_rp_cnt;
  unsigned dir_rp_cnt;
  unsigned err_cnt;
  FileTotals() {
    memset(this, 0, sizeof(*this));
  }
};

struct FileAnalyzer {
  ObjectArray<UnicodeString> file_list;
  FileInfo file_info;
  ObjectArray<FileInfo> hard_links;
  NtfsVolume volume;
  FileTotals totals;
  HANDLE h_dlg;
  Array<FarDialogItem> dlg_items;
#ifdef FARAPI18
  ObjectArray<UnicodeString> dlg_text;
#endif
  Array<CHAR_INFO> dlg_ci;
  CtrlIds ctrl;
  HANDLE h_thread;
  HANDLE h_stop_event;
  time_t update_timer;
  void display_file_info(bool partial = false);
  void update_totals(const FileInfo& file_info, bool hard_link);
  void process_file(FileInfo& file_info, bool full_info);
  void process_recursive(const UnicodeString& dir_name);
  void process();
  static unsigned __stdcall th_proc(void* param);
  LONG_PTR dialog_handler(int msg, int param1, LONG_PTR param2);
  static LONG_PTR WINAPI dlg_proc(HANDLE h_dlg, int msg, int param1, LONG_PTR param2);
  FileAnalyzer(): h_dlg(NULL), h_thread(NULL), h_stop_event(NULL), update_timer(0) {
  }
  ~FileAnalyzer() {
    if (h_stop_event) CloseHandle(h_stop_event);
    if (h_thread) CloseHandle(h_thread);
  }
};

Array<CHAR_INFO> str_to_char_info(const FarStr& str) {
  Array<CHAR_INFO> out;
  WORD attr;
  out.extend(str.size());
  CHAR_INFO ci;
  for (unsigned i = 0; i < str.size(); i++) {
    if (str[i] == '\1') {
      i++;
      attr = str[i];
      continue;
    }
#ifdef FARAPI17
    ci.Char.AsciiChar = str[i];
#endif
#ifdef FARAPI18
    ci.Char.UnicodeChar = str[i];
#endif
    ci.Attributes = attr;
    out.add(ci);
  }
  return out;
}

unsigned fmt_char_cnt(const UnicodeString& str) {
  unsigned cnt = 0;
  for (unsigned i = 0; i < str.size(); i++) {
    if (str[i] == '\1') {
      cnt += 2;
      i++;
      continue;
    }
  }
  return cnt;
}

void FileAnalyzer::display_file_info(bool partial) {
  const UnicodeString empty_str;

  /* flag - show size summary for directories / multiple files */
  bool show_totals = (file_info.directory && !file_info.reparse) || (file_list.size() > 1);
  /* flag - single file is selected */
  bool single_file = file_list.size() == 1;
  /* current control index */
  unsigned ctrl_idx = 0;
  /* dialog line counter */
  unsigned line = 0;
  /* current hot key index */
  unsigned hot_key_idx = 0;

  if (h_dlg != NULL) g_far.SendDlgMessage(h_dlg, DM_ENABLEREDRAW, FALSE, 0);

  /* FAR window size */
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
  if (con == INVALID_HANDLE_VALUE) FAIL(SystemError());
  CONSOLE_SCREEN_BUFFER_INFO con_info;
  if (GetConsoleScreenBufferInfo(con, &con_info) == 0) FAIL(SystemError());
  unsigned con_width = con_info.srWindow.Right - con_info.srWindow.Left + 1;
  unsigned con_height = con_info.srWindow.Bottom - con_info.srWindow.Top + 1;

  /* fixed width columns */
  const unsigned c_col_type_len = 21;
  const unsigned c_col_flags_len = 5;
  /* variable width columns */
  unsigned col_data_size_min_len = far_get_msg(MSG_METADATA_COL_DATA_SIZE).size();
  unsigned col_data_size_max_len = col_data_size_min_len;
  unsigned col_data_size_len;
  unsigned col_disk_size_min_len = far_get_msg(MSG_METADATA_COL_DISK_SIZE).size();
  unsigned col_disk_size_max_len = col_disk_size_min_len;
  unsigned col_disk_size_len;
  unsigned col_fragments_min_len = far_get_msg(MSG_METADATA_COL_FRAGMENTS).size();
  unsigned col_fragments_max_len = col_fragments_min_len;
  unsigned col_fragments_len;
  unsigned col_name_min_len = far_get_msg(MSG_METADATA_COL_NAME_DATA).size();
  unsigned col_name_max_len = col_name_min_len;
  unsigned col_name_len;

  UnicodeString file_name_label = far_get_msg(MSG_METADATA_FILE_NAME);
  UnicodeString file_name_text =  L" " + file_name_label + L" ";
  for (unsigned p; ((p = file_name_text.search(L'&')) != -1);) file_name_text.remove(p);

  /* horizontal dialog border size */
  const unsigned c_hframe_width = 3;

  /* minimum allowed dialog width */
  unsigned min_dlg_width = c_hframe_width + 1 + c_col_type_len + 1 + c_col_flags_len + 1 +
    col_data_size_min_len + 1 + col_disk_size_min_len + 1 + col_fragments_min_len + 1 + col_name_min_len +
    1 + c_hframe_width;

  if (con_width < min_dlg_width) FAIL(MsgError(L"Console window size is too small"));

  /* useful macro to measure required column width */
  UnicodeString tmp;
#define MEASURE_COLUMN(col_size, value) { \
  tmp = format_inf_amount(value); \
  if (tmp.size() > col_size) col_size = tmp.size(); \
  tmp = format_inf_amount_short(value); \
  if (tmp.size() > col_size) col_size = tmp.size(); \
}
#define MEASURE_COLUMN2(col_size, value1, value2) { \
  if (value2 == 0) { \
    MEASURE_COLUMN(col_size, value1); \
  } \
  else { \
    tmp = format_inf_amount(value1) + '+' + format_inf_amount(value2); \
    if (tmp.size() > col_size) col_size = tmp.size(); \
    tmp = format_inf_amount_short(value1) + '+' + format_inf_amount_short(value2); \
    if (tmp.size() > col_size) col_size = tmp.size(); \
  } \
}

  u64 total_data_size = 0; // file total non-resident data size
  u64 total_disk_size = 0; // file total non-resident disk size
  if (single_file) {
    for (unsigned i = 0; i < file_info.attr_list.size(); i++) {
      const AttrInfo& attr = file_info.attr_list[i];
      MEASURE_COLUMN(col_data_size_max_len, attr.data_size);
      if (!attr.resident) {
        total_data_size += attr.data_size;
        total_disk_size += attr.disk_size;

        MEASURE_COLUMN(col_disk_size_max_len, attr.disk_size);
        MEASURE_COLUMN(col_fragments_max_len, attr.fragments);
      }
      if (attr.name.size() + 2 > col_name_max_len) col_name_max_len = attr.name.size() + 2;
    }
    MEASURE_COLUMN(col_data_size_max_len, total_data_size);
    MEASURE_COLUMN(col_disk_size_max_len, total_disk_size);
  }
  if (show_totals) {
    MEASURE_COLUMN2(col_data_size_max_len, totals.unnamed_data_size, totals.unnamed_hl_size);
    MEASURE_COLUMN(col_disk_size_max_len, totals.unnamed_disk_size);
    MEASURE_COLUMN2(col_data_size_max_len, totals.named_data_size, totals.named_hl_size);
    MEASURE_COLUMN(col_disk_size_max_len, totals.named_disk_size);
    MEASURE_COLUMN2(col_data_size_max_len, totals.nr_data_size, totals.nr_hl_size);
    MEASURE_COLUMN(col_disk_size_max_len, totals.nr_disk_size);
    MEASURE_COLUMN(col_fragments_max_len, totals.excess_fragments);
  }

  /* max. width of attribute table */
  unsigned attr_table_width = c_hframe_width + 1 + c_col_type_len + 1 + c_col_flags_len +
    1 + col_data_size_max_len + 1 + col_disk_size_max_len + 1 + col_fragments_max_len +
    1 + col_name_max_len + 1 + c_hframe_width;
  /* max. width of file information panel */
  unsigned info_panel_width = c_hframe_width + 1 + file_name_text.size() +
    (single_file ? (file_info.file_name.size() + 1) : far_get_msg(MSG_METADATA_MULTIPLE).size()) + 1 + c_hframe_width;
  /* max. dialog width */
  unsigned max_dlg_width = attr_table_width > info_panel_width ? attr_table_width : info_panel_width;

#define DIALOG_WIDTH (c_hframe_width + 1 + c_col_type_len + 1 + c_col_flags_len + \
    1 + col_data_size_len + 1 + col_disk_size_len + 1 + col_fragments_len + \
    1 + col_name_len + 1 + c_hframe_width)

  /* calculate real dialog width */
  unsigned dlg_width;
  if (max_dlg_width > con_width) {
    dlg_width = con_width;
    col_data_size_len = col_data_size_min_len;
    col_disk_size_len = col_disk_size_min_len;
    col_fragments_len = col_fragments_min_len;
    col_name_len = col_name_min_len;
    /* try to maximize size columns first */
    while (DIALOG_WIDTH < dlg_width) {
      bool no_change = true;
      if (col_data_size_len < col_data_size_max_len) {
        col_data_size_len++;
        no_change = false;
        if (DIALOG_WIDTH == dlg_width) break;
      }
      if (col_disk_size_len < col_disk_size_max_len) {
        col_disk_size_len++;
        no_change = false;
        if (DIALOG_WIDTH == dlg_width) break;
      }
      if (col_fragments_len < col_fragments_max_len) {
        col_fragments_len++;
        no_change = false;
        if (DIALOG_WIDTH == dlg_width) break;
      }
      if (no_change) break;
    }
  }
  else {
    dlg_width = max_dlg_width;
    col_data_size_len = col_data_size_max_len;
    col_disk_size_len = col_disk_size_max_len;
    col_fragments_len = col_fragments_max_len;
    col_name_len = col_name_max_len;
  }
  /* set Name/Data column to max. possible size */
  while (DIALOG_WIDTH < dlg_width) col_name_len++;

  /* column X coord. */
  unsigned col_data_size_x = c_hframe_width + 1 + c_col_type_len + 1 + c_col_flags_len + 1;
  unsigned col_disk_size_x = c_hframe_width + 1 + c_col_type_len + 1 + c_col_flags_len + 1 +
    col_data_size_len + 1;
  unsigned col_fragments_x = c_hframe_width + 1 + c_col_type_len + 1 + c_col_flags_len + 1 +
    col_data_size_len + 1 + col_disk_size_len + 1;
  unsigned col_name_x = c_hframe_width + 1 + c_col_type_len + 1 + c_col_flags_len + 1 +
    col_data_size_len + 1 + col_disk_size_len + 1 + col_fragments_len + 1;

  /* width of the dialog client area */
  unsigned dlg_cl_width = dlg_width - 2 * c_hframe_width - 2;

  UnicodeString dlg_str;
  dlg_str.set_inc(dlg_width * 2);
  FarDialogItem di;

  /* useful macros to form dialog contents */
#ifdef FARAPI17
#define SET_DATA(str) { \
  AnsiString text = unicode_to_oem(str); \
  if (text.size() >= sizeof(di.Data)) { \
    strncpy(di.Data, text.data(), sizeof(di.Data)); \
    di.Data[sizeof(di.Data) - 1] = 0; \
  } \
  else strcpy(di.Data, text.data()); \
}
#endif
#ifdef FARAPI18
#define SET_DATA(str) \
  if (h_dlg == NULL) { \
    dlg_text += str; \
    di.PtrData = dlg_text.last().data(); \
  } \
  else { \
    dlg_text.item(ctrl_idx) = str; \
    di.PtrData = dlg_text[ctrl_idx].data(); \
  }
#endif

#define ADD_STR_LINE(str) { \
  unsigned fmt_cnt = fmt_char_cnt(str); \
  dlg_str.add_fmt(L"\1%c%.*c\1%c%c\1%c%-*.*S\1%c%c\1%c%.*c", g_colors[COL_DIALOGTEXT], c_hframe_width, ' ', \
    g_colors[COL_DIALOGBOX], c_vert2, g_colors[COL_DIALOGTEXT], dlg_cl_width + fmt_cnt, dlg_cl_width + fmt_cnt, &(str), \
    g_colors[COL_DIALOGBOX], c_vert2, g_colors[COL_DIALOGTEXT], c_hframe_width, ' '); \
  line++; \
}

#define ADD_HORIZ_LINE(chl, chm, chr, cho) { \
  dlg_str.add_fmt(L"\1%c%.*c\1%c%c%.*c%c%.*c%c%.*c%c%.*c%c%.*c%c%.*c%c\1%c%.*c", \
    g_colors[COL_DIALOGTEXT], c_hframe_width, ' ', g_colors[COL_DIALOGBOX], \
    chl, c_col_type_len, cho, chm, c_col_flags_len, cho, chm, \
    col_data_size_len, cho, chm, col_disk_size_len, cho, chm, \
    col_fragments_len, cho, chm, col_name_len, cho, chr, \
    g_colors[COL_DIALOGTEXT], c_hframe_width, ' '); \
  line++; \
}

#define ADD_TABLE_LINE(type, flags, data_size, disk_size, fragments, name) { \
  ADD_STR_LINE(UnicodeString::format(L"%*S\1%c%c\1%c%*S\1%c%c\1%c%*.*S\1%c%c\1%c%*.*S\1%c%c\1%c%*.*S\1%c%c\1%c%-*.*S", \
    c_col_type_len, &(type), g_colors[COL_DIALOGBOX], c_vert1, g_colors[COL_DIALOGTEXT], \
    c_col_flags_len, &(flags), g_colors[COL_DIALOGBOX], c_vert1, g_colors[COL_DIALOGTEXT], \
    col_data_size_len, col_data_size_len, &(data_size), g_colors[COL_DIALOGBOX], c_vert1, g_colors[COL_DIALOGTEXT], \
    col_disk_size_len, col_disk_size_len, &(disk_size), g_colors[COL_DIALOGBOX], c_vert1, g_colors[COL_DIALOGTEXT], \
    col_fragments_len, col_fragments_len, &(fragments), g_colors[COL_DIALOGBOX], c_vert1, g_colors[COL_DIALOGTEXT], \
    col_name_len, col_name_len, &(name))); \
}

#define ADD_CTRL(val, x, len, show) { \
  memset(&di, 0, sizeof(di)); \
  di.Type = DI_EDIT; \
  di.Flags = DIF_READONLY | DIF_SELECTONENTRY; \
  di.X1 = (x); \
  di.X2 = di.X1 + (len) - 1; \
  di.Y1 = di.Y2 = line; \
  SET_DATA(val); \
  if (h_dlg != NULL) { \
    g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di); \
    g_far.SendDlgMessage(h_dlg, DM_SHOWITEM, ctrl_idx, (val.size() > (len)) && (show) ? 1 : 0); \
  } \
  else { \
    dlg_items += di; \
  } \
  ctrl_idx++; \
}

// edit control + hot key label for Name/Data column
#define ADD_ND_CTRL(val, show) { \
  bool active = (val.size() != 0) && (show); \
\
  memset(&di, 0, sizeof(di)); \
  di.Type = DI_TEXT; \
  di.X1 = col_name_x; \
  di.X2 = di.X1; \
  di.Y1 = di.Y2 = line; \
  if (hot_key_idx <= 8) { \
    SET_DATA(UnicodeString::format(L"&%u", hot_key_idx + 1)); \
  } \
  else { \
    SET_DATA(empty_str); \
  } \
  if (h_dlg != NULL) { \
    g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di); \
    g_far.SendDlgMessage(h_dlg, DM_SHOWITEM, ctrl_idx, active ? 1 : 0); \
  } \
  else { \
    dlg_items += di; \
  } \
  ctrl_idx++; \
  if (active) hot_key_idx++; \
\
  memset(&di, 0, sizeof(di)); \
  di.Type = DI_EDIT; \
  di.Flags = DIF_READONLY | DIF_SELECTONENTRY; \
  di.X1 = col_name_x + 1; \
  di.X2 = di.X1 + col_name_len - 2; \
  di.Y1 = di.Y2 = line; \
  SET_DATA(val); \
  if (h_dlg != NULL) { \
    g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di); \
    g_far.SendDlgMessage(h_dlg, DM_SHOWITEM, ctrl_idx, active ? 1 : 0); \
  } \
  else { \
    dlg_items += di; \
  } \
  ctrl_idx++; \
}

#define ADD_CTRL_LINE(data_size, disk_size, fragments, name, show) { \
  ADD_CTRL(data_size, col_data_size_x, col_data_size_len, show); \
  ADD_CTRL(disk_size, col_disk_size_x, col_disk_size_len, show); \
  ADD_CTRL(fragments, col_fragments_x, col_fragments_len, show); \
  ADD_ND_CTRL(name, show); \
}

#define ADD_SIZE_TOTALS(title, data_size, hl_size, disk_size, fragments) { \
  UnicodeString data_size_fmt; \
  UnicodeString data_size_fmt_short; \
  if ((hl_size) == 0) { \
    data_size_fmt = format_inf_amount(data_size); \
    data_size_fmt_short = format_inf_amount_short(data_size); \
  } \
  else { \
    data_size_fmt = format_inf_amount(data_size) + '+' + format_inf_amount(hl_size); \
    data_size_fmt_short = format_inf_amount_short(data_size) + '+' + format_inf_amount_short(hl_size); \
  } \
  UnicodeString disk_size_fmt = format_inf_amount(disk_size); \
  UnicodeString disk_size_fmt_short = format_inf_amount_short(disk_size); \
  UnicodeString fragments_fmt = (fragments) == 0 ? empty_str : UnicodeString::format(far_get_msg(MSG_METADATA_TOTALS_EXCESS_FRAGMENTS).data(), &format_inf_amount(fragments)); \
  ADD_CTRL_LINE(data_size_fmt, disk_size_fmt, fragments_fmt, empty_str, true); \
  ADD_TABLE_LINE(title, empty_str, data_size_fmt, disk_size_fmt, fragments_fmt, empty_str); \
  bool second_line = (data_size_fmt != data_size_fmt_short) || (disk_size_fmt != disk_size_fmt_short); \
  ADD_CTRL_LINE(data_size_fmt_short, disk_size_fmt_short, empty_str, empty_str, second_line); \
  if (second_line) ADD_TABLE_LINE(UnicodeString(), empty_str, data_size_fmt_short, disk_size_fmt_short, empty_str, empty_str); \
}

  // reserve place for table
  if (h_dlg == NULL) {
    dlg_items = di;
#ifdef FARAPI18
    dlg_text = UnicodeString();
#endif
  }
  ctrl_idx++;

  /* dialog border */
  dlg_str.add_fmt(L"\1%c%.*c", g_colors[COL_DIALOGTEXT], dlg_width, ' ');
  line++;

  /* dialog header */
  unsigned plugin_title_len = far_get_msg(MSG_PLUGIN_NAME).size();
  if (plugin_title_len > dlg_cl_width - 2) plugin_title_len = dlg_cl_width - 2;
  unsigned hl1 = (dlg_cl_width - plugin_title_len) / 2;
  unsigned hl2 = dlg_cl_width - plugin_title_len - hl1;
  if (hl1 != 0) hl1--;
  if (hl2 != 0) hl2--;
  dlg_str.add_fmt(L"\1%c%.*c\1%c%c%.*c \1%c%.*s\1%c %.*c%c\1%c%.*c", g_colors[COL_DIALOGTEXT], c_hframe_width, ' ',
    g_colors[COL_DIALOGBOX], c_top2_left2, hl1, c_horiz2, g_colors[COL_DIALOGBOXTITLE], plugin_title_len, far_get_msg(MSG_PLUGIN_NAME).data(),
    g_colors[COL_DIALOGBOX], hl2, c_horiz2, c_top2_right2, g_colors[COL_DIALOGTEXT], c_hframe_width, ' ');

  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.X1 = c_hframe_width + 1 + hl1 + 1;
  di.X2 = di.X1 + far_get_msg(MSG_PLUGIN_NAME).size() - 1;
  di.Y1 = di.Y2 = line;
  SET_DATA(far_get_msg(MSG_PLUGIN_NAME));
  if (h_dlg != NULL) {
    g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di);
  }
  else {
    dlg_items += di;
  }
  ctrl_idx++;
  line++;

  /* file information panel */
  if (single_file) {
    /* file name read-only EDIT control + hot key label */
    if (h_dlg == NULL) {
      memset(&di, 0, sizeof(di));
      di.Type = DI_TEXT;
      di.X1 = c_hframe_width + 1 + 1;
      di.X2 = di.X1 + get_label_len(file_name_label) - 1;
      di.Y1 = di.Y2 = line;
      SET_DATA(file_name_label);
      dlg_items += di;
    }
    ctrl_idx++;

    memset(&di, 0, sizeof(di));
    di.Type = DI_EDIT;
    di.Flags = DIF_READONLY | DIF_SELECTONENTRY;
    di.X1 = c_hframe_width + 1 + file_name_text.size();
    di.X2 = di.X1 + dlg_cl_width - file_name_text.size() - 1;
    di.Y1 = di.Y2 = line;
    di.Focus = 1;
    SET_DATA(file_info.file_name);
    if (h_dlg != NULL) {
      g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di);
    }
    else {
      dlg_items += di;
    }
    ctrl_idx++;

    ADD_STR_LINE(file_name_text + file_info.file_name);

    /* other file info */
    UnicodeString str;
    if (file_info.mft_rec_cnt > 1) {
      str.add(L" ").add_fmt(far_get_msg(MSG_METADATA_MFT_RECS).data(), file_info.mft_rec_cnt);
    }
    if (file_info.hard_link_cnt > 1) {
      str.add(L" ").add_fmt(far_get_msg(MSG_METADATA_HARD_LINKS).data(), file_info.hard_link_cnt);
    }
    if (str.size() != 0) ADD_STR_LINE(str);

  }
  else {
    /* multiple files are selected */
    ADD_STR_LINE(file_name_text + far_get_msg(MSG_METADATA_MULTIPLE));
  }

  ADD_HORIZ_LINE(c_left2_horiz1, c_top1_vert1, c_right2_horiz1, c_horiz1);

  /* attribute table header */
  ADD_TABLE_LINE(far_get_msg(MSG_METADATA_COL_TYPE), far_get_msg(MSG_METADATA_COL_FLAGS), far_get_msg(MSG_METADATA_COL_DATA_SIZE),
    far_get_msg(MSG_METADATA_COL_DISK_SIZE), far_get_msg(MSG_METADATA_COL_FRAGMENTS), far_get_msg(MSG_METADATA_COL_NAME_DATA));

  if (single_file) {
    ADD_HORIZ_LINE(c_left2_horiz1, c_cross1, c_right2_horiz1, c_horiz1);

    /* attribute list */
    UnicodeString flags;
    for (unsigned i = 0; i < file_info.attr_list.size(); i++) {
      const AttrInfo& attr = file_info.attr_list[i];

      /* attribute type name */
      UnicodeString attr_type = attr.type_name();

      /* attribute flags */
      flags.copy(attr.resident ? 'R' : ' ').add(attr.mft_ext_rec ? 'X' : ' ').add(attr.compressed ? 'C' : ' ').
        add(attr.encrypted ? 'E' : ' ').add(attr.sparse ? 'S' : ' ');

      /* attribute table line */
      UnicodeString data_size_fmt = format_inf_amount(attr.data_size);
      UnicodeString disk_size_fmt;
      UnicodeString fragments_fmt;
      UnicodeString data_size_fmt_short = format_inf_amount_short(attr.data_size);
      UnicodeString disk_size_fmt_short;
      if (!attr.resident) {
        disk_size_fmt = format_inf_amount(attr.disk_size);
        fragments_fmt = format_inf_amount(attr.fragments);
        disk_size_fmt_short = format_inf_amount_short(attr.disk_size);
      }
      ADD_CTRL_LINE(data_size_fmt, disk_size_fmt, fragments_fmt, attr.name, true);
      ADD_TABLE_LINE(attr_type, flags, data_size_fmt, disk_size_fmt, fragments_fmt, attr.name);
      bool second_line = (data_size_fmt != data_size_fmt_short) || (disk_size_fmt != disk_size_fmt_short); \
      ADD_CTRL_LINE(data_size_fmt_short, disk_size_fmt_short, empty_str, empty_str, second_line);
      if (second_line) ADD_TABLE_LINE(UnicodeString(), empty_str, data_size_fmt_short, disk_size_fmt_short, empty_str, empty_str);
    }

    /* attribute table totals */
    ADD_HORIZ_LINE(c_left2_horiz1, c_cross1, c_right2_horiz1, c_horiz1);
    ADD_SIZE_TOTALS(far_get_msg(MSG_METADATA_ROW_NR_TOTAL), total_data_size, 0, total_disk_size, 0);
  }

  /* display totals for directory / multiple files */
  if (show_totals) {
    ADD_HORIZ_LINE(c_left2_horiz1, c_bottom1_vert1, c_right2_horiz1, c_horiz1);

    /* summary: count of files, dirs, etc. */
    UnicodeString str;
    if (partial) str.add_fmt(L" \1%c*\1%c", g_colors[COL_DIALOGHIGHLIGHTTEXT], g_colors[COL_DIALOGTEXT]);
    if ((totals.file_cnt != 0) || (totals.file_rp_cnt != 0)) {
      str.add(L" ").add_fmt(far_get_msg(MSG_METADATA_TOTALS_FILES).data(), totals.file_cnt);
    }
    if (totals.hl_cnt != 0) {
      str.add_fmt(far_get_msg(MSG_METADATA_TOTALS_HL).data(), totals.hl_cnt);
    }
    if (totals.file_rp_cnt != 0) {
      str.add_fmt(far_get_msg(MSG_METADATA_TOTALS_RP_FILES).data(), totals.file_rp_cnt);
    }
    if ((totals.dir_cnt != 0) || (totals.dir_rp_cnt != 0)) {
      str.add(L" ").add_fmt(far_get_msg(MSG_METADATA_TOTALS_DIRS).data(), totals.dir_cnt);
    }
    if (totals.dir_rp_cnt != 0) {
      str.add_fmt(far_get_msg(MSG_METADATA_TOTALS_RP_DIRS).data(), totals.dir_rp_cnt);
    }
    if (totals.err_cnt != 0) {
      str.add(L" ").add_fmt(far_get_msg(MSG_METADATA_TOTALS_ERRORS).data(), L'\1', CHANGE_FG(g_colors[COL_DIALOGTEXT], FOREGROUND_RED), totals.err_cnt, L'\1', g_colors[COL_DIALOGTEXT]);
    }
    ADD_STR_LINE(str);

    /* total size of directory contents / multiple files */
    ADD_HORIZ_LINE(c_left2_horiz1, c_top1_vert1, c_right2_horiz1, c_horiz1);
    ADD_SIZE_TOTALS(far_get_msg(MSG_METADATA_ROW_UNNAMED_TOTAL), totals.unnamed_data_size, totals.unnamed_hl_size, totals.unnamed_disk_size, 0);
    ADD_SIZE_TOTALS(far_get_msg(MSG_METADATA_ROW_NAMED_TOTAL), totals.named_data_size, totals.named_hl_size, totals.named_disk_size, 0);
    ADD_SIZE_TOTALS(far_get_msg(MSG_METADATA_ROW_NR_TOTAL), totals.nr_data_size, totals.nr_hl_size, totals.nr_disk_size, totals.excess_fragments);
  }

  /* link control */
  if (ctrl.linked != -1) {
    ADD_HORIZ_LINE(c_left2_horiz1, c_bottom1_vert1, c_right2_horiz1, c_horiz1);
  }
  else {
    ADD_HORIZ_LINE(c_bottom2_left2, c_bottom2_vert1, c_bottom2_right2, c_horiz2);
  }

  // hot key
  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.X1 = c_hframe_width + 1;
  di.X2 = di.X1;
  di.Y1 = di.Y2 = line;
  SET_DATA(UnicodeString(L"&0"));
  if (h_dlg != NULL) {
    g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di);
    g_far.SendDlgMessage(h_dlg, DM_SHOWITEM, ctrl_idx, ctrl.linked != -1 ? 1 : 0);
  }
  else {
    dlg_items += di;
  }
  ctrl_idx++;

  memset(&di, 0, sizeof(di));
  di.Type = DI_EDIT;
  di.Flags = DIF_READONLY | DIF_SELECTONENTRY;
  di.X1 = c_hframe_width + 2;
  di.X2 = di.X1 + dlg_cl_width - 2;
  di.Y1 = di.Y2 = line;
  if ((ctrl.linked != -1) && (h_dlg != NULL)) {
    FarStr str;
    unsigned len = (unsigned) g_far.SendDlgMessage(h_dlg, DM_GETTEXTLENGTH, ctrl.linked, 0);
    g_far.SendDlgMessage(h_dlg, DM_GETTEXTPTR, ctrl.linked, (LONG_PTR) str.buf(len));
    str.set_size(len);
    SET_DATA(FARSTR_TO_UNICODE(str));
  }
  else {
    SET_DATA(empty_str);
  }
  if (h_dlg != NULL) {
    g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di);
    g_far.SendDlgMessage(h_dlg, DM_SHOWITEM, ctrl_idx, ctrl.linked != -1 ? 1 : 0);
  }
  else {
    dlg_items += di;
    ctrl.link = ctrl_idx;
  }
  ctrl_idx++;

  if (ctrl.linked != -1) {
    ADD_HORIZ_LINE(c_vert2, ' ', c_vert2, ' ');
    ADD_HORIZ_LINE(c_bottom2_left2, c_horiz2, c_bottom2_right2, c_horiz2);
  }

  /* dialog border */
  dlg_str.add_fmt(L"\1%c%.*c", g_colors[COL_DIALOGTEXT], dlg_width, ' ');
  line++;

  /* custom control - dialog contents */
  ctrl_idx = 0;
  memset(&di, 0, sizeof(di));
  di.Type = DI_USERCONTROL;
  di.Flags = DIF_NOFOCUS;
  di.X1 = 0;
  di.Y1 = 0;
  di.X2 = dlg_width - 1;
  di.Y2 = line - 1;
  dlg_ci = str_to_char_info(UNICODE_TO_FARSTR(dlg_str));
  di.VBuf = (CHAR_INFO*) dlg_ci.data();
  if (h_dlg != NULL) {
    g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_idx, (long) &di);
  }
  else {
    dlg_items.item(0) = di;
  }

  unsigned dlg_height = line;

  if (h_dlg != NULL) {
    SMALL_RECT dlg_rect;
    g_far.SendDlgMessage(h_dlg, DM_GETDLGRECT, 0, (long) &dlg_rect);
    if ((dlg_rect.Right - dlg_rect.Left + 1 != dlg_width) || (dlg_rect.Bottom - dlg_rect.Top + 1 != dlg_height)) {
      COORD dlg_size;
      dlg_size.X = dlg_width;
      dlg_size.Y = dlg_height;
      g_far.SendDlgMessage(h_dlg, DM_RESIZEDIALOG, 0, (long) &dlg_size);
      COORD dlg_pos = { -1, -1 };
      g_far.SendDlgMessage(h_dlg, DM_MOVEDIALOG, TRUE, (long) &dlg_pos);
    }
    g_far.SendDlgMessage(h_dlg, DM_ENABLEREDRAW, TRUE, 0);
  }
  else {
#ifdef FARAPI17
    g_far.DialogEx(g_far.ModuleNumber, -1, -1, dlg_width, dlg_height, "metadata", (FarDialogItem*) dlg_items.data(), dlg_items.size(), 0, 0, dlg_proc, reinterpret_cast<LONG_PTR>(this));
#endif // FARAPI17
#ifdef FARAPI18
    HANDLE h = g_far.DialogInit(g_far.ModuleNumber, -1, -1, dlg_width, dlg_height, L"metadata", (FarDialogItem*) dlg_items.data(), dlg_items.size(), 0, 0, dlg_proc, reinterpret_cast<LONG_PTR>(this));
    if (h != INVALID_HANDLE_VALUE) {
      g_far.DialogRun(h);
      g_far.DialogFree(h);
    }
#endif // FARAPI18
  }
}

void FileAnalyzer::update_totals(const FileInfo& file_info, bool hard_link) {
  if (file_info.reparse && file_info.directory) totals.dir_rp_cnt++;
  else if (file_info.directory) totals.dir_cnt++;
  else if (hard_link) totals.hl_cnt++;
  else if (file_info.reparse) totals.file_rp_cnt++;
  else totals.file_cnt++;
  for (unsigned i = 0; i < file_info.attr_list.size(); i++) {
    const AttrInfo& attr_info = file_info.attr_list[i];
    if (!attr_info.resident) {
      if (hard_link) {
        totals.nr_hl_size += attr_info.data_size;
      }
      else {
        totals.nr_data_size += attr_info.data_size;
        totals.nr_disk_size += attr_info.disk_size;
      }
    }
    if (attr_info.type == AT_DATA) {
      if (attr_info.name.size() == 0) {
        if (hard_link) {
          totals.unnamed_hl_size += attr_info.data_size;
        }
        else {
          totals.unnamed_data_size += attr_info.data_size;
          totals.unnamed_disk_size += attr_info.disk_size;
        }
      }
      else {
        if (hard_link) {
          totals.named_hl_size += attr_info.data_size;
        }
        else {
          totals.named_data_size += attr_info.data_size;
          totals.named_disk_size += attr_info.disk_size;
        }
      }
    }
    if (!hard_link && (attr_info.fragments > 1)) totals.excess_fragments += attr_info.fragments - 1;
  }
}

void FileAnalyzer::process_file(FileInfo& file_info, bool full_info) {
  BY_HANDLE_FILE_INFORMATION h_file_info;
  HANDLE h_file = CreateFileW(long_path(file_info.file_name).data(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, NULL);
  if (h_file == INVALID_HANDLE_VALUE) FAIL(SystemError());
  try {
    if (GetFileInformationByHandle(h_file, &h_file_info) == 0) FAIL(SystemError());
  }
  finally (CloseHandle(h_file));

  u64 file_ref_num = ((u64) h_file_info.nFileIndexHigh << 32) + h_file_info.nFileIndexLow;
  file_info.hard_link_cnt = h_file_info.nNumberOfLinks;
  file_info.reparse = (h_file_info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT;
  file_info.directory = (h_file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
  if (volume.serial != h_file_info.dwVolumeSerialNumber) { // volume changed
    volume.open(extract_path_root(get_real_path(extract_file_dir(file_info.file_name))));
  }
  file_info.volume = &volume;

  if (file_info.hard_link_cnt > 1) {
    unsigned idx = hard_links.bsearch(file_info);
    if (idx != -1) {
      update_totals(hard_links[idx], true);
    }
    else {
      file_info.process_file(file_ref_num);
      if (full_info) file_info.find_full_paths();
      update_totals(file_info, false);
      hard_links += file_info;
      hard_links.sort();
    }
  }
  else {
    file_info.process_file(file_ref_num);
    if (full_info) file_info.find_full_paths();
    update_totals(file_info, false);
  }
}

void FileAnalyzer::process_recursive(const UnicodeString& dir_name) {
  try {
    bool root_dir = dir_name.last() == L'\\';
    WIN32_FIND_DATAW find_data;
    HANDLE h_find = FindFirstFileW(long_path(dir_name + (root_dir ? L"*" : L"\\*")).data(), &find_data);
    if (h_find == INVALID_HANDLE_VALUE) FAIL(SystemError());
    try {
      while (true) {
        if (WaitForSingleObject(h_stop_event, 0) == WAIT_OBJECT_0) break;
        if (FindNextFileW(h_find, &find_data) == 0) {
          if (GetLastError() == ERROR_NO_MORE_FILES) break;
          else FAIL(SystemError());
        }
        if ((wcscmp(find_data.cFileName, L".") == 0) || (wcscmp(find_data.cFileName, L"..") == 0)) continue;

        FileInfo file_info;
        file_info.file_name = dir_name + (root_dir ? L"" : L"\\") + find_data.cFileName;
        try {
          process_file(file_info, false);
          if (file_info.directory && !file_info.reparse) {
            process_recursive(file_info.file_name);
          }
        }
        catch (Error&) {
          totals.err_cnt++;
        }

        time_t ctime = time(NULL);
        if (ctime > update_timer + c_update_time) {
          display_file_info(true);
          update_timer = ctime;
        }
      }
    }
    finally (FindClose(h_find));
  }
  catch (Error&) {
    totals.err_cnt++;
  }
}

// set current file on active panel to file_name
bool panel_go_to_file(const FarStr& file_name) {
  FarStr dir = file_name;
  if ((dir.size() >= 4) && (dir.equal(0, 4, FAR_T("\\\\?\\")) || dir.equal(0, 4, FAR_T("\\??\\")))) dir.remove(0, 4);
  if ((dir.size() < 2) || (dir[1] != ':')) return false;
  unsigned pos = dir.rsearch('\\');
  if (pos == -1) return false;
  FarStr file = dir.slice(pos + 1, dir.size() - pos - 1);
  dir.remove(pos, dir.size() - pos);
  if (dir.size() == 2) dir += '\\';
  if (!far_control_ptr(INVALID_HANDLE_VALUE, FCTL_SETPANELDIR, dir.data())) return false;
  PanelInfo panel_info;
  if (!far_control_ptr(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &panel_info)) return false;
  PanelRedrawInfo panel_ri;
  int i;
  for (i = 0; i < panel_info.ItemsNumber; i++) {
    PluginPanelItem* ppi = far_get_panel_item(INVALID_HANDLE_VALUE, i, panel_info);
    if (!ppi) return false;
    if (file == FAR_FILE_NAME(ppi->FindData)) {
      panel_ri.CurrentItem = i;
      panel_ri.TopPanelItem = 0;
      break;
    }
  }
  if (i == panel_info.ItemsNumber) return false;
  if (!far_control_ptr(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, &panel_ri)) return false;
  return true;
}

#ifdef FARAPI17
int WINAPI GetMinFarVersion(void) {
  return MAKEFARVERSION(1, 71, 2411);
}
#endif
#ifdef FARAPI18
int WINAPI GetMinFarVersion(void) {
  return FARMANAGERVERSION;
}
int WINAPI GetMinFarVersionW(void) {
  return FARMANAGERVERSION;
}
#endif

void WINAPI FAR_EXPORT(SetStartupInfo)(const struct PluginStartupInfo* info) {
  g_far = *info;
  g_fsf = *info->FSF;
  g_far.FSF = &g_fsf;
  g_version = get_module_version(g_h_module);
}

const FarCh* c_command_prefix = FAR_T("nfi:nfc:defrag");
void WINAPI FAR_EXPORT(GetPluginInfo)(struct PluginInfo *pi) {
  static const FarCh* plugin_menu[1];

  pi->StructSize = sizeof(struct PluginInfo);

  pi->Flags = PF_FULLCMDLINE;
  plugin_menu[0] = far_msg_ptr(MSG_PLUGIN_NAME);
  pi->PluginMenuStrings = plugin_menu;
  pi->PluginMenuStringsNumber = sizeof(plugin_menu) / sizeof(plugin_menu[0]);
  pi->PluginConfigStrings = plugin_menu;
  pi->PluginConfigStringsNumber = sizeof(plugin_menu) / sizeof(plugin_menu[0]);
  pi->CommandPrefix = c_command_prefix;
}

void FileAnalyzer::process() {
  if (file_info.directory && !file_info.reparse) {
    process_recursive(file_info.file_name);
  }
  for (unsigned i = 1; i < file_list.size(); i++) {
    FileInfo fi;
    fi.file_name = file_list[i];
    try {
      process_file(fi, false);
      if (fi.directory && !fi.reparse) {
        process_recursive(fi.file_name);
      }
    }
    catch (Error&) {
      totals.err_cnt++;
    }
    time_t ctime = time(NULL);
    if (ctime > update_timer + c_update_time) {
      display_file_info(true);
      update_timer = ctime;
    }
  }
  display_file_info();
}

unsigned __stdcall FileAnalyzer::th_proc(void* param) {
  try {
    reinterpret_cast<FileAnalyzer*>(param)->process();
  }
  catch (Error& e) {
    error_dlg(e);
    return FALSE;
  }
  return TRUE;
}

LONG_PTR FileAnalyzer::dialog_handler(int msg, int param1, LONG_PTR param2) {
  const MOUSE_EVENT_RECORD* mouse_evt = (const MOUSE_EVENT_RECORD*) param2;
  /* do we need background processing? */
  bool bg_proc = (file_info.directory && !file_info.reparse) || (file_list.size() > 1);
  if (msg == DN_INITDIALOG) {
    NOFAIL(display_file_info());
    if (bg_proc) {
      h_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
      CHECK_SYS(h_stop_event != NULL);
      unsigned th_id;
      h_thread = (HANDLE) _beginthreadex(NULL, 0, th_proc, this, 0, &th_id);
      CHECK_SYS(h_thread != NULL);
    }
    return TRUE;
  }
  else if (msg == DN_CLOSE) {
    if (bg_proc) {
      SetEvent(h_stop_event);
      if (WaitForSingleObject(h_thread, 0) == WAIT_OBJECT_0) {
        return TRUE;
      }
      else return FALSE;
    }
    else return TRUE;
  }
  else if (((msg == DN_MOUSECLICK) && (param1 != -1) && (mouse_evt->dwEventFlags == DOUBLE_CLICK) &&
    ((mouse_evt->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) == FROM_LEFT_1ST_BUTTON_PRESSED)) ||
    ((msg == DN_KEY) && (param2 == KEY_ENTER))) {

    unsigned ctrl_id = param1;
    if ((dlg_items[ctrl_id].Type == DI_EDIT) || (dlg_items[ctrl_id].Type == DI_FIXEDIT)) {
      if (ctrl_id == ctrl.linked) {
        ctrl.linked = -1;
        NOFAIL(display_file_info());
      }
      else if (ctrl_id == ctrl.link) {
        FarStr file_name;
        unsigned len = (unsigned) g_far.SendDlgMessage(h_dlg, DM_GETTEXTLENGTH, ctrl.link, 0);
        g_far.SendDlgMessage(h_dlg, DM_GETTEXTPTR, ctrl.link, (LONG_PTR) file_name.buf(len));
        file_name.set_size(len);
        if (panel_go_to_file(file_name)) g_far.SendDlgMessage(h_dlg, DM_CLOSE, -1, 0);
      }
      else {
        ctrl.linked = ctrl_id;
        NOFAIL(display_file_info());
      }
      return TRUE;
    }
    else return FALSE;
  }
  else if (msg == DN_RESIZECONSOLE) {
    NOFAIL(display_file_info());
    return TRUE;
  }
  else return g_far.DefDlgProc(h_dlg, msg, param1, param2);
}

LONG_PTR WINAPI FileAnalyzer::dlg_proc(HANDLE h_dlg, int msg, int param1, LONG_PTR param2) {
  BEGIN_ERROR_HANDLER;
  FileAnalyzer* fa = reinterpret_cast<FileAnalyzer*>(g_far.SendDlgMessage(h_dlg, DM_GETDLGDATA, 0, 0));
  fa->h_dlg = h_dlg;
  return fa->dialog_handler(msg, param1, param2);
  END_ERROR_HANDLER(;,;);
  return g_far.DefDlgProc(h_dlg, msg, param1, param2);
}

// get file name from command line (when plugin prefix is used)
bool file_list_from_cmdline(const UnicodeString& cmd, ObjectArray<UnicodeString>& file_list, UnicodeString& prefix) {
  unsigned prefix_pos = cmd.search(L':');
  if (prefix_pos == -1) return false;
  prefix = cmd.left(prefix_pos);
  UnicodeString file_name = cmd.slice(prefix_pos + 1);
  unquote(file_name);
  UnicodeString full_file_name;
  LPWSTR file_part;
  DWORD ret = GetFullPathNameW(file_name.data(), MAX_PATH, full_file_name.buf(MAX_PATH), &file_part);
  if (ret > MAX_PATH) {
    ret = GetFullPathNameW(file_name.data(), ret, full_file_name.buf(ret), &file_part);
  }
  CHECK_SYS(ret);
  full_file_name.set_size(ret);
  file_list = full_file_name;
  return true;
}

UnicodeString get_unicode_file_path(const PluginPanelItem& panel_item, const UnicodeString& dir_path, bool own_panel) {
#ifdef FARAPI17
  if (own_panel) {
    if (panel_item.UserData) {
      u32 name_size = (*reinterpret_cast<const u32*>(panel_item.UserData) - sizeof(u32)) / sizeof(wchar_t);
      const wchar_t* name_ptr = reinterpret_cast<const wchar_t*>(reinterpret_cast<const unsigned char*>(panel_item.UserData) + sizeof(u32));
      return dir_path + UnicodeString(name_ptr, name_size);
    }
    else {
      return dir_path + oem_to_unicode(panel_item.FindData.cFileName);
    }
  }
  else {
    UnicodeString file_path = dir_path;
    UnicodeString file_name = oem_to_unicode(panel_item.FindData.cFileName);
    unsigned fn_pos = file_name.rsearch(L'\\');
    bool is_path = fn_pos != -1;
    if (is_path) {
      // if far_find_data contains path (plugin panel)
      file_path = file_path + file_name.left(fn_pos + 1);
      file_name.remove(0, fn_pos + 1);
    }
    bool fn_invalid = file_name.search(L'?') != -1; // Far uses '?' instead of unmappable characters
    WIN32_FIND_DATAW find_data;
    HANDLE h_find = fn_invalid ? INVALID_HANDLE_VALUE : FindFirstFileW(long_path(file_path + file_name).data(), &find_data);
    if (h_find == INVALID_HANDLE_VALUE) {
      // try using short file name if present
      if (!is_path && *panel_item.FindData.cAlternateFileName) {
        h_find = FindFirstFileW(long_path(file_path + oem_to_unicode(panel_item.FindData.cAlternateFileName)).data(), &find_data);
      }
    }
    if (h_find != INVALID_HANDLE_VALUE) {
      VERIFY(FindClose(h_find) != 0);
      return file_path + find_data.cFileName;
    }
    else {
      return file_path + file_name;
    }
  }
#endif
#ifdef FARAPI18
  return dir_path + panel_item.FindData.lpwszFileName;
#endif
}

// get list of the select files from panel and place it into global file list
bool file_list_from_panel(ObjectArray<UnicodeString>& file_list, bool own_panel) {
  if (!far_control_int(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0)) return false;
  PanelInfo p_info;
  if (!far_control_ptr(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &p_info)) return false;
  bool file_panel = (p_info.PanelType == PTYPE_FILEPANEL) && ((p_info.Flags & PFLAGS_REALNAMES) == PFLAGS_REALNAMES);
  bool tree_panel = (p_info.PanelType == PTYPE_TREEPANEL) && ((p_info.Flags & PFLAGS_REALNAMES) == PFLAGS_REALNAMES);
  if (!file_panel && !tree_panel) return false;

  unsigned sel_file_cnt = 0;
  if (file_panel) {
    for (int i = 0; i < p_info.SelectedItemsNumber; i++) {
      PluginPanelItem* ppi = far_get_selected_panel_item(INVALID_HANDLE_VALUE, i, p_info);
      if (!ppi) return false;
      if ((ppi->Flags & PPIF_SELECTED) == PPIF_SELECTED) sel_file_cnt++;
    }
  }

  file_list.clear();
  UnicodeString cur_dir = FARSTR_TO_UNICODE(far_get_cur_dir(INVALID_HANDLE_VALUE, p_info));
  if (sel_file_cnt != 0) {
    file_list.extend(sel_file_cnt);
    cur_dir = add_trailing_slash(cur_dir);
    for (int i = 0; i < p_info.SelectedItemsNumber; i++) {
      PluginPanelItem* ppi = far_get_selected_panel_item(INVALID_HANDLE_VALUE, i, p_info);
      if (!ppi) return false;
      if ((ppi->Flags & PPIF_SELECTED) == PPIF_SELECTED) {
        file_list += get_unicode_file_path(*ppi, cur_dir, own_panel);
      }
    }
  }
  else {
    if (file_panel) {
      if ((p_info.CurrentItem < 0) || (p_info.CurrentItem >= p_info.ItemsNumber)) return false;
      PluginPanelItem* ppi = far_get_panel_item(INVALID_HANDLE_VALUE, p_info.CurrentItem, p_info);
      if (!ppi) return false;
      if (FAR_STRCMP(FAR_FILE_NAME(ppi->FindData), FAR_T("..")) == 0) { // current directory selected
        if (cur_dir.size() == 0) return false; // directory is invalid (plugin panel)
        else file_list += cur_dir;
      }
      else {
        cur_dir = add_trailing_slash(cur_dir);
        file_list += get_unicode_file_path(*ppi, cur_dir, own_panel);
      }
    }
    else { // Tree panel
      file_list += cur_dir;
    }
  }

  file_list.sort();
  return true;
}

enum FileType {
  ftFile,
  ftDirectory,
  ftReparse,
};

FileType get_file_type(const UnicodeString& file_name) {
  DWORD attr = GetFileAttributesW(file_name.data());
  if (attr == INVALID_FILE_ATTRIBUTES) return ftFile;
  if ((attr & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT) return ftReparse;
  else if ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) return ftDirectory;
  else return ftFile;
}

/* show file information dialog */
void plugin_show_metadata(const ObjectArray<UnicodeString>& file_list) {
  FileAnalyzer fa;
  fa.file_list = file_list;
  fa.volume.open(extract_path_root(get_real_path(extract_file_dir(file_list[0]))));
  fa.file_info.file_name = file_list[0];
  fa.process_file(fa.file_info, true);
  fa.display_file_info();
}

void plugin_process_contents(const ObjectArray<UnicodeString>& file_list) {
  bool single_file = (file_list.size() == 1) && (get_file_type(file_list[0]) == ftFile);
  if (show_options_dialog(g_content_options, single_file)) {
    store_plugin_options();
    if (single_file) {
      ContentInfo content_info;
      process_file_content(file_list[0], g_content_options, content_info);
      show_result_dialog(file_list[0], g_content_options, content_info);
    }
    else {
      CompressionStats stats;
      compress_files(file_list, stats);
      show_result_dialog(stats);
    }
  }
}

HANDLE WINAPI FAR_EXPORT(OpenPlugin)(int OpenFrom, INT_PTR item) {
  HANDLE handle = INVALID_HANDLE_VALUE;
  BEGIN_ERROR_HANDLER;
  /* load plugin configuration settings from registry */
  load_suffixes();
  load_plugin_options();

  /* load Far color table */
  unsigned colors_size = (unsigned) g_far.AdvControl(g_far.ModuleNumber, ACTL_GETARRAYCOLOR, NULL);
  g_far.AdvControl(g_far.ModuleNumber, ACTL_GETARRAYCOLOR, g_colors.buf(colors_size));
  g_colors.set_size(colors_size);

  ObjectArray<UnicodeString> file_list;

  if (OpenFrom == OPEN_COMMANDLINE) {
    UnicodeString prefix;
    if (file_list_from_cmdline(FARSTR_TO_UNICODE((const FarCh*) item), file_list, prefix)) {
      if (prefix == L"nfi") plugin_show_metadata(file_list);
      else if (prefix == L"nfc") plugin_process_contents(file_list);
      else if (prefix == L"defrag") {
        defragment(file_list, Log());
        far_control_int(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, 1);
        far_control_int(PANEL_PASSIVE, FCTL_UPDATEANOTHERPANEL, 1);
        far_control_ptr(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, NULL);
        far_control_ptr(PANEL_PASSIVE, FCTL_REDRAWANOTHERPANEL, NULL);
      }
    }
  }
  else { // OPEN_PLUGINSMENU
    // check if current panel belongs to our plugin
    FilePanel* active_panel = FilePanel::get_active_panel();

    /* display plugin menu */
    ObjectArray<UnicodeString> menu_items;
    menu_items += far_get_msg(MSG_MENU_METADATA);
    menu_items += far_get_msg(MSG_MENU_CONTENT);
    menu_items += far_get_msg(active_panel == NULL ? MSG_MENU_PANEL_ON : MSG_MENU_PANEL_OFF);
    menu_items += far_get_msg(MSG_MENU_DEFRAGMENT);
    if (active_panel != NULL) menu_items += far_get_msg(active_panel->flat_mode ? MSG_MENU_FLAT_MODE_OFF : MSG_MENU_FLAT_MODE_ON);
    if (active_panel != NULL) menu_items += far_get_msg(active_panel->mft_mode ? MSG_MENU_MFT_MODE_OFF : MSG_MENU_MFT_MODE_ON);
    int item_idx = far_menu(far_get_msg(MSG_PLUGIN_NAME), menu_items, FAR_T("plugin_menu"));

    if (item_idx == 0) {
      if (file_list_from_panel(file_list, active_panel != NULL)) {
        plugin_show_metadata(file_list);
      }
    }
    else if (item_idx == 1) {
      if (file_list_from_panel(file_list, active_panel != NULL)) {
        plugin_process_contents(file_list);
      }
    }
    else if (item_idx == 2) {
      if (active_panel == NULL) {
        handle = FilePanel::open();
      }
      else {
        active_panel->close();
      }
    }
    else if (item_idx == 3) {
      ObjectArray<UnicodeString> file_list;
      if (file_list_from_panel(file_list, active_panel != NULL)) {
        Log log;
        defragment(file_list, log);
        if (log.size() != 0) {
          if (far_message(far_get_msg(MSG_PLUGIN_NAME) + L"\n" + word_wrap(far_get_msg(MSG_DEFRAG_ERRORS), get_msg_width()) + L"\n" + far_get_msg(MSG_BUTTON_OK) + L"\n" + far_get_msg(MSG_LOG_SHOW), 2, FMSG_WARNING) == 1) log.show();
        }
        far_control_int(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, 1);
        far_control_int(PANEL_PASSIVE, FCTL_UPDATEANOTHERPANEL, 1);
      }
    }
    else if (item_idx == 4) {
      active_panel->flat_mode = !active_panel->flat_mode;
      far_control_int(active_panel, FCTL_UPDATEPANEL, 1);
    }
    else if (item_idx == 5) {
      active_panel->toggle_mft_mode();
      far_control_int(active_panel, FCTL_UPDATEPANEL, 1);
    }
  }
  END_ERROR_HANDLER(return handle, return INVALID_HANDLE_VALUE);
}

void WINAPI FAR_EXPORT(ClosePlugin)(HANDLE hPlugin) {
  BEGIN_ERROR_HANDLER;
  FilePanel* panel = (FilePanel*) hPlugin;
  panel->on_close();
  END_ERROR_HANDLER(;,;);
}

void WINAPI FAR_EXPORT(GetOpenPluginInfo)(HANDLE hPlugin, struct OpenPluginInfo* Info) {
  BEGIN_ERROR_HANDLER;
  FilePanel* panel = (FilePanel*) hPlugin;
  panel->fill_plugin_info(Info);
  END_ERROR_HANDLER(;,;);
}

int WINAPI FAR_EXPORT(GetFindData)(HANDLE hPlugin, struct PluginPanelItem** pPanelItem, int* pItemsNumber, int OpMode) {
  BEGIN_ERROR_HANDLER;
  FilePanel* panel = (FilePanel*) hPlugin;
  try {
    panel->new_file_list(*pPanelItem, *pItemsNumber, (OpMode & OPM_FIND) != 0);
  }
  catch (...) {
    if (OpMode & (OPM_FIND | OPM_SILENT)) BREAK;
    throw;
  }
  END_ERROR_HANDLER(return TRUE, return FALSE);
}

void WINAPI FAR_EXPORT(FreeFindData)(HANDLE hPlugin, struct PluginPanelItem* PanelItem, int ItemsNumber) {
  BEGIN_ERROR_HANDLER;
  FilePanel* panel = (FilePanel*) hPlugin;
  panel->clear_file_list(PanelItem);
  END_ERROR_HANDLER(;,;);
}

int WINAPI FAR_EXPORT(SetDirectory)(HANDLE hPlugin, const FarCh* Dir, int OpMode) {
  BEGIN_ERROR_HANDLER;
  FilePanel* panel = (FilePanel*) hPlugin;
  try {
    panel->change_directory(FARSTR_TO_UNICODE(FarStr(Dir)), (OpMode & OPM_FIND) != 0);
  }
  catch (...) {
    if (OpMode & (OPM_FIND | OPM_SILENT)) BREAK;
    throw;
  }
  END_ERROR_HANDLER(return TRUE, return FALSE);
}

int WINAPI FAR_EXPORT(Configure)(int ItemNumber) {
  BEGIN_ERROR_HANDLER;
  load_plugin_options();
  FilePanelMode file_panel_mode = g_file_panel_mode;
  if (show_file_panel_mode_dialog(file_panel_mode)) {
    bool need_reload = (file_panel_mode.show_streams != g_file_panel_mode.show_streams) || (file_panel_mode.show_main_stream != g_file_panel_mode.show_main_stream);
    g_file_panel_mode = file_panel_mode;
    store_plugin_options();
    if (need_reload) FilePanel::reload_mft_all();
  }
  else return FALSE;
  END_ERROR_HANDLER(return TRUE, return FALSE);
}

int WINAPI FAR_EXPORT(ProcessKey)(HANDLE hPlugin, int Key, unsigned int ControlState) {
  BEGIN_ERROR_HANDLER;
  FilePanel* panel = (FilePanel*) hPlugin;
  if ((Key == VK_F24) && (ControlState == (PKF_CONTROL | PKF_ALT | PKF_SHIFT))) {
    panel->apply_saved_state();
  }
  else if ((Key == VK_F3) && (ControlState == 0)) {
    PanelInfo pi;
    if (!far_control_ptr(panel, FCTL_GETPANELINFO, &pi)) return FALSE;
    if ((pi.CurrentItem < 0) || (pi.CurrentItem >= pi.ItemsNumber)) return FALSE;
    PluginPanelItem* ppi = far_get_panel_item(panel, pi.CurrentItem, pi);
    if (!ppi) return false;
    if ((ppi->FindData.dwFileAttributes & FILE_ATTR_DIRECTORY) == 0) return FALSE;
    ObjectArray<UnicodeString> file_list;
    if (FAR_STRCMP(FAR_FILE_NAME(ppi->FindData), FAR_T("..")) == 0) { // current directory selected
      file_list = panel->get_current_dir();
    }
    else {
      file_list = get_unicode_file_path(*ppi, add_trailing_slash(panel->get_current_dir()), true);
    }
    plugin_show_metadata(file_list);
  }
  else if ((Key == 'R') && (ControlState == PKF_CONTROL)) {
    if (!g_file_panel_mode.use_usn_journal) panel->reload_mft();
    return FALSE;
  }
  else return FALSE;
  END_ERROR_HANDLER(return TRUE, return TRUE);
}

int WINAPI FAR_EXPORT(Compare)(HANDLE hPlugin, const struct PluginPanelItem *Item1, const struct PluginPanelItem *Item2, unsigned int Mode) {
  if (Mode == SM_NAME) return FAR_STRCMP(FAR_FILE_NAME(Item1->FindData), FAR_FILE_NAME(Item2->FindData));
  else return -2;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    g_h_module = hinstDLL;
  }
  return TRUE;
}
