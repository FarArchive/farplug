#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "utils.h"
#include "dlgapi.h"

#ifdef FARAPI18
#  define DIF_VAREDIT 0
#endif

extern struct PluginStartupInfo g_far;
extern Array<unsigned char> g_colors;

// dialog frame size
const unsigned c_x_frame = 5;
const unsigned c_y_frame = 2;

// frame characters
const wchar_t c_horiz1 = 9472;
const wchar_t c_vert1 = 9474;
const wchar_t c_horiz2 = 9552;
const wchar_t c_vert2 = 9553;
const wchar_t c_top2_left2 = 9556;
const wchar_t c_top2_right2 = 9559;
const wchar_t c_bottom2_left2 = 9562;
const wchar_t c_bottom2_right2 = 9565;
const wchar_t c_left2_horiz1 = 9567;
const wchar_t c_right2_horiz1 = 9570;
const wchar_t c_top1_vert1 = 9516;
const wchar_t c_bottom1_vert1 = 9524;
const wchar_t c_bottom2_vert1 = 9575;
const wchar_t c_cross1 = 9532;

// progress bar chars
const wchar_t c_pb_black = 9608;
const wchar_t c_pb_white = 9617;

UnicodeString far_get_msg(int id) {
  return FARSTR_TO_UNICODE(g_far.GetMsg(g_far.ModuleNumber, id));
}

const FarCh* far_msg_ptr(int id) {
  return g_far.GetMsg(g_far.ModuleNumber, id);
}

// optimum client area width of message dialogs
unsigned get_msg_width() {
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
  if (con != INVALID_HANDLE_VALUE) {
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    if (GetConsoleScreenBufferInfo(con, &con_info) != 0) {
      unsigned con_width = con_info.srWindow.Right - con_info.srWindow.Left + 1;
      if (con_width >= 80) return con_width - 20;
    }
  }
  return 60;
}

int far_message(const UnicodeString& msg, int button_cnt, DWORD flags) {
  return g_far.Message(g_far.ModuleNumber, flags | FMSG_ALLINONE, NULL, (const FarCh* const*) UNICODE_TO_FARSTR(msg).data(), 0, button_cnt);
}

int far_menu(const UnicodeString& title, const ObjectArray<UnicodeString>& items, const FarCh* help) {
  Array<FarMenuItem> menu_items;
  FarMenuItem mi;
  for (unsigned i = 0; i < items.size(); i++) {
    memset(&mi, 0, sizeof(mi));
#ifdef FARAPI17
    FAR_STRCPY(mi.Text, unicode_to_oem(items[i]).data());
#endif // FARAPI17
#ifdef FARAPI18
    mi.Text = items[i].data();
#endif // FARAPI18
    menu_items += mi;
  }
  return g_far.Menu(g_far.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, UNICODE_TO_FARSTR(title).data(), NULL, help, NULL, NULL, menu_items.data(), menu_items.size());
}

int far_viewer(const UnicodeString& file_name, const UnicodeString& title) {
  return g_far.Viewer(UNICODE_TO_FARSTR(file_name).data(), UNICODE_TO_FARSTR(title).data(), 0, 0, -1, -1, VF_DISABLEHISTORY | VF_ENABLE_F6 | VF_NONMODAL
#ifdef FARAPI18
  , CP_UNICODE
#endif // FARAPI18
  );
}

void far_text(unsigned x, unsigned y, char cl, const UnicodeString& text) {
  g_far.Text(x, y, cl, UNICODE_TO_FARSTR(text).data());
}

unsigned get_label_len(const UnicodeString& str) {
  unsigned cnt = 0;
  for (unsigned i = 0; i < str.size(); i++) {
    if (str[i] != '&') cnt++;
  }
  return cnt;
}

// number of characters in the formatted string
unsigned get_fmt_str_len(const UnicodeString& str) {
  unsigned cnt = 0;
  for (unsigned i = 0; i < str.size(); i++) {
    if (str[i] == '\1') {
      i++;
      continue;
    }
    else if (str[i] == '\2') {
      continue;
    }
    else cnt++;
  }
  return cnt;
}

// draw string with embedded color information
void draw_fmt_str(unsigned x, unsigned y, const UnicodeString& str, unsigned xs) {
  const char def_cl = g_colors[COL_DIALOGTEXT]; // default color attr
  unsigned char cl = def_cl; // current color attr
  unsigned p = 0; // current character string start
  if (xs == -1) xs = get_fmt_str_len(str);
  unsigned i;
  for (i = 0; (i < str.size()) && (xs != 0); i++) {
    // if color attribute marker
    if (str[i] == '\1') { // set specific color
      unsigned l = i - p; // current string length
      // if character string length != 0
      if (l != 0) {
        // draw previous string segment
        if (l > xs) l = xs;
        far_text(x, y, cl, str.slice(p, l));
        x += l;
        xs -= l;
      }
      p = i + 2; // jump to next character string
      i++; // skip color marker
      cl = (char) str[i]; // update color value
    }
    else if (str[i] == '\2') { // set default color
      unsigned l = i - p; // current string length
      // if character string length != 0
      if (l != 0) {
        // draw previous string segment
        if (l > xs) l = xs;
        far_text(x, y, cl, str.slice(p, l));
        x += l;
        xs -= l;
      }
      p = i + 1; // jump to next character string
      cl = def_cl; // update color value
    }
  }
  if (p < i) {
    unsigned l = i - p;
    if (l > xs) l = xs;
    far_text(x, y, cl, str.slice(p, l));
  }
}

UnicodeString char_info_to_fmt_str(const Array<CHAR_INFO>& char_info) {
  UnicodeString fmt_str;
  char prev_attr;
  for (unsigned i = 0; i < char_info.size(); i++) {
    if ((i == 0) || (prev_attr != char_info[i].Attributes)) {
      prev_attr = LOBYTE(char_info[i].Attributes);
      fmt_str.add('\1').add(prev_attr);
    }
    fmt_str.add(char_info[i].Char.UnicodeChar);
  }
  return fmt_str;
}

void draw_frame(unsigned x0, unsigned y0, unsigned w, unsigned h, const UnicodeString& title) {
  UnicodeString str;
  if ((w == 0) || (h == 0)) return;
  bool no_frame = (w < 2 * (c_x_frame - 1)) || (h < 2 * c_y_frame);

  // if there is no space to draw frame
  if (no_frame) {
    str.copy_fmt(L"%.*c", w, ' ');
    for (unsigned y = y0; y < y0 + h; y++) {
      far_text(x0, y, g_colors[COL_DIALOGTEXT], str);
    }
  }
  // draw frame
  else {
    // top border
    str.copy_fmt(L"%.*c", w, ' ');
    for (unsigned i = 0; i + 1 < c_y_frame; i++) {
      far_text(x0, y0 + i, g_colors[COL_DIALOGTEXT], str);
    }

    // internal area dimensions
    unsigned in_w = w - 2 * (c_x_frame - 1);
    unsigned in_h = h - 2 * c_y_frame;

    // title bar
    unsigned title_len = title.size() + 2; // plus two adjacent spaces
    if (in_w < title_len) title_len = in_w;

    unsigned tl1 = (in_w - title_len) / 2; // frame len before title
    unsigned tl2 = in_w - title_len - tl1; // frame len after title

    str.copy_fmt(L"\1%c%.*c\1%c%c%.*c\1%c%.*S\1%c%.*c%c\1%c%.*c",
      g_colors[COL_DIALOGTEXT], c_x_frame - 2, ' ',
      g_colors[COL_DIALOGBOX], c_top2_left2, tl1, c_horiz2,
      g_colors[COL_DIALOGBOXTITLE], title_len, &(' ' + title + ' '),
      g_colors[COL_DIALOGBOX], tl2, c_horiz2, c_top2_right2,
      g_colors[COL_DIALOGTEXT], c_x_frame - 2, ' ');
    draw_fmt_str(x0, y0 + c_y_frame - 1, str);

    // vertical borders
    for (unsigned y = y0 + c_y_frame; y < y0 + c_y_frame + in_h; y++) {
      str.copy_fmt(L"\1%c%.*c\1%c%c\1%c%.*c\1%c%c\1%c%.*c",
        g_colors[COL_DIALOGTEXT], c_x_frame - 2, ' ',
        g_colors[COL_DIALOGBOX], c_vert2,
        g_colors[COL_DIALOGTEXT], in_w, ' ',
        g_colors[COL_DIALOGBOX], c_vert2,
        g_colors[COL_DIALOGTEXT], c_x_frame - 2, ' ');
      draw_fmt_str(x0, y, str);
    }

    str.copy_fmt(L"\1%c%.*c\1%c%c%.*c%c\1%c%.*c",
      g_colors[COL_DIALOGTEXT], c_x_frame - 2, ' ',
      g_colors[COL_DIALOGBOX], c_bottom2_left2, in_w, c_horiz2, c_bottom2_right2,
      g_colors[COL_DIALOGTEXT], c_x_frame - 2, ' ');
    draw_fmt_str(x0, y0 + c_y_frame + in_h, str);

    // bottom border
    str.copy_fmt(L"%.*c", w, ' ');
    for (unsigned i = 0; i + 1 < c_y_frame; i++) {
      far_text(x0, y0 + c_y_frame + in_h + 1 + i, g_colors[COL_DIALOGTEXT], str);
    }
  }

  // draw shadow
#define SHADOW_MASK (~FOREGROUND_INTENSITY & ~BACKGROUND_BLUE & ~BACKGROUND_GREEN & ~BACKGROUND_RED & ~BACKGROUND_INTENSITY)
  HANDLE h_con = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD coord = { 0, 0 };
  COORD size = { w, 1 };
  SMALL_RECT rect = { x0 + 2, y0 + h, x0 + 2 + size.X, y0 + h + size.Y };
  Array<CHAR_INFO> char_info;
  ReadConsoleOutputW(h_con, char_info.buf(size.X), size, coord, &rect);
  char_info.set_size(size.X);
  for (unsigned i = 0; i < char_info.size(); i++) {
    char_info.item(i).Attributes = char_info[i].Attributes & SHADOW_MASK;
  }
  str = char_info_to_fmt_str(char_info);
  draw_fmt_str(rect.Left, rect.Top, str);
  for (unsigned y = y0 + 1; y <= y0 + h; y++) {
    COORD size = { 2, 1 };
    SMALL_RECT rect = { x0 + w, y, x0 + w + size.X, y + size.Y };
    ReadConsoleOutputW(h_con, char_info.buf(size.X), size, coord, &rect);
    char_info.set_size(size.X);
    for (unsigned i = 0; i < char_info.size(); i++) {
      char_info.item(i).Attributes = char_info[i].Attributes & SHADOW_MASK;
    }
    str = char_info_to_fmt_str(char_info);
    draw_fmt_str(rect.Left, rect.Top, str);
  }
}

void draw_text_box(const UnicodeString& title, const ObjectArray<UnicodeString>& lines, unsigned client_xs) {
  // console size
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
  CHECK_SYS(con != INVALID_HANDLE_VALUE);
  CONSOLE_SCREEN_BUFFER_INFO con_info;
  CHECK_SYS(GetConsoleScreenBufferInfo(con, &con_info) != 0);
  unsigned con_xs = con_info.srWindow.Right - con_info.srWindow.Left + 1;
  unsigned con_ys = con_info.srWindow.Bottom - con_info.srWindow.Top + 1;

  // dialog dimensions
  unsigned client_ys = lines.size();
  unsigned dlg_xs = client_xs + 2 * c_x_frame;
  unsigned dlg_ys = client_ys + 2 * c_y_frame;

  // dialog top-left corner
  unsigned dlg_x = (con_xs - dlg_xs) / 2;
  unsigned dlg_y = (con_ys - dlg_ys) / 2;

  // client area top-left corner
  unsigned cl_x = dlg_x + c_x_frame;
  unsigned cl_y = dlg_y + c_y_frame;

  // draw dialog frame
  draw_frame(dlg_x, dlg_y, dlg_xs, dlg_ys, title);

  // draw dialog client area
  for (unsigned y = 0; y < client_ys; y++) {
    draw_fmt_str(cl_x, cl_y + y, lines[y], client_xs);
  }

  // flush Far screen buffer
  g_far.Text(0, 0, 0, NULL);
}

void FarDialog::set_text(FarDialogItem& di, const UnicodeString& text) {
#ifdef FARAPI17
  if (text.size() < sizeof(di.Data)) strcpy(di.Data, unicode_to_oem(text).data());
  else strcpy(di.Data, unicode_to_oem(text.left(sizeof(di.Data) - 1)).data());
#endif
#ifdef FARAPI18
  values += text;
  di.PtrData = values.last().data();
#endif
}

void FarDialog::set_buf(FarDialogItem& di, const UnicodeString& text, unsigned bufsize) {
#ifdef FARAPI17
  if (di.Type == DI_FIXEDIT) {
    set_text(di, text);
  }
  else {
    values += unicode_to_oem(text);
    di.Ptr.PtrData = values.last_item().buf(bufsize);
    di.Ptr.PtrLength = bufsize;
  }
#endif
#ifdef FARAPI18
  set_text(di, text);
#endif
}

void FarDialog::set_list_text(FarListItem& li, const UnicodeString& text) {
#ifdef FARAPI17
  if (text.size() < sizeof(li.Text)) strcpy(li.Text, unicode_to_oem(text).data());
  else strcpy(li.Text, unicode_to_oem(text.left(sizeof(li.Text) - 1)).data());
#endif
#ifdef FARAPI18
  values += text;
  li.Text = values.last().data();
#endif
}

void FarDialog::frame(const UnicodeString& text) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_DOUBLEBOX;
  di.X1 = c_x_frame - 2;
  di.Y1 = c_y_frame - 1;
  di.X2 = c_x_frame + client_xs + 1;
  di.Y2 = c_y_frame + client_ys;
  set_text(di, text);
  new_item(di);
}

void FarDialog::calc_frame_size() {
  client_ys = y - c_y_frame;
  FarDialogItem& di = items.item(index[0]); // dialog frame
  di.X2 = c_x_frame + client_xs + 1;
  di.Y2 = c_y_frame + client_ys;
}

unsigned FarDialog::new_item(const FarDialogItem& di) {
  items += di;
  index += items.size() - 1;
  return index.size() - 1;
}

FarDialog::FarDialog(const UnicodeString& title, unsigned width): client_xs(width), x(c_x_frame), y(c_y_frame) {
  frame(title);
}

void FarDialog::new_line() {
  x = c_x_frame;
  y++;
}

void FarDialog::spacer(unsigned size) {
  x += size;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
}

unsigned FarDialog::separator() {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.Y1 = y;
  di.Y2 = y;
  di.Flags = DIF_SEPARATOR;
  return new_item(di);
}

unsigned FarDialog::separator(const UnicodeString& text) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.Y1 = y;
  di.Y2 = y;
  di.Flags = DIF_SEPARATOR;
  set_text(di, text);
  return new_item(di);
}

unsigned FarDialog::label(const UnicodeString& text, unsigned boxsize, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x += get_label_len(text);
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  di.Flags = flags;
  set_text(di, text);
  return new_item(di);
}

unsigned FarDialog::var_edit_box(const UnicodeString& text, unsigned bufsize, unsigned boxsize, DWORD flags) {
  // bufsize is not used with FARAPI18
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_EDIT;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x = c_x_frame + client_xs;
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  di.Flags = DIF_VAREDIT | flags;
  set_buf(di, text, bufsize);
  return new_item(di);
}

unsigned FarDialog::mask_edit_box(const UnicodeString& text, const UnicodeString& mask, unsigned boxsize, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_FIXEDIT;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x += mask.size();
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  values += UNICODE_TO_FARSTR(mask);
  di.Mask = values.last().data();
  di.Flags = DIF_MASKEDIT | flags;
  set_buf(di, text, boxsize);
  return new_item(di);
}

unsigned FarDialog::fix_edit_box(const UnicodeString& text, unsigned boxsize, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_FIXEDIT;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x += text.size();
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  di.Flags = flags;
  set_buf(di, text, boxsize);
  return new_item(di);
}

unsigned FarDialog::button(const UnicodeString& text, DWORD flags, bool def) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_BUTTON;
  di.X1 = x;
  di.Y1 = y;
  x += get_label_len(text) + 4;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.Y2 = y;
  di.Flags = flags;
  di.DefaultButton = def ? 1 : 0;
  set_text(di, text);
  return new_item(di);
}

unsigned FarDialog::check_box(const UnicodeString& text, int value, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_CHECKBOX;
  di.X1 = x;
  di.Y1 = y;
  x += get_label_len(text) + 4;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.Y2 = y;
  di.Flags = flags;
  di.Selected = value;
  set_text(di, text);
  return new_item(di);
}

unsigned FarDialog::radio_button(const UnicodeString& text, bool value, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_RADIOBUTTON;
  di.X1 = x;
  di.Y1 = y;
  x += get_label_len(text) + 4;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.Y2 = y;
  di.Flags = flags;
  di.Selected = value ? 1 : 0;
  set_text(di, text);
  return new_item(di);
}

unsigned FarDialog::combo_box(const ObjectArray<UnicodeString>& list_items, unsigned sel_idx, unsigned bufsize, unsigned boxsize, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_COMBOBOX;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x = c_x_frame + client_xs;
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 2;
  di.Y2 = y;
  di.Flags = DIF_VAREDIT | flags;
  ListData list_data;
  for (unsigned i = 0; i < list_items.size(); i++) {
    FarListItem li;
    memset(&li, 0, sizeof(li));
    set_list_text(li, list_items[i]);
    if (i == sel_idx) li.Flags = LIF_SELECTED;
    list_data.items += li;
  }
  FarList fl;
  fl.ItemsNumber = list_data.items.size();
  fl.Items = const_cast<FarListItem*>(list_data.items.data());
  list_data.list = fl;
  lists += list_data;
  di.ListItems = const_cast<FarList*>(lists.last().list.data());
  set_buf(di, L"", bufsize);
  return new_item(di);
}

int FarDialog::show(FARWINDOWPROC dlg_proc, const FarCh* help) {
  calc_frame_size();
#ifdef FARAPI17
  return g_far.DialogEx(g_far.ModuleNumber, -1, -1, client_xs + 2 * c_x_frame, client_ys + 2 * c_y_frame, help, items.buf(), items.size(), 0, 0, dlg_proc, (LONG_PTR) this);
#endif // FARAPI17
#ifdef FARAPI18
  int res = -1;
  HANDLE h_dlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, client_xs + 2 * c_x_frame, client_ys + 2 * c_y_frame, help, items.buf(), items.size(), 0, 0, dlg_proc, (LONG_PTR) this);
  if (h_dlg != INVALID_HANDLE_VALUE) {
    res = g_far.DialogRun(h_dlg);
    g_far.DialogFree(h_dlg);
  }
  return res;
#endif // FARAPI18
}

FarDialog* FarDialog::get_dlg(HANDLE h_dlg) {
  FarDialog* dlg = (FarDialog*) g_far.SendDlgMessage(h_dlg, DM_GETDLGDATA, 0, 0);
  dlg->h_dlg = h_dlg;
  return dlg;
}

UnicodeString FarDialog::get_text(unsigned ctrl_id) {
  FarStr str;
  unsigned len = (unsigned) g_far.SendDlgMessage(h_dlg, DM_GETTEXTLENGTH, index[ctrl_id], 0);
  g_far.SendDlgMessage(h_dlg, DM_GETTEXTPTR, index[ctrl_id], (LONG_PTR) str.buf(len));
  str.set_size(len);
  return FARSTR_TO_UNICODE(str);
}

void FarDialog::set_text(unsigned ctrl_id, const UnicodeString& text) {
  g_far.SendDlgMessage(h_dlg, DM_SETTEXTPTR, index[ctrl_id], (LONG_PTR) UNICODE_TO_FARSTR(text).data());
}

bool FarDialog::get_check(unsigned ctrl_id) {
  return DlgItem_GetCheck(g_far, h_dlg, index[ctrl_id]) == BSTATE_CHECKED;
}

void FarDialog::set_check(unsigned ctrl_id, bool check) {
  g_far.SendDlgMessage(h_dlg, DM_SETCHECK, index[ctrl_id], check ? BSTATE_CHECKED : BSTATE_UNCHECKED);
}

unsigned FarDialog::get_list_pos(unsigned ctrl_id) {
  return (unsigned) g_far.SendDlgMessage(h_dlg, DM_LISTGETCURPOS, index[ctrl_id], 0);
}

void FarDialog::set_color(unsigned ctrl_id, unsigned char color) {
#ifdef FARAPI17
  FarDialogItem buf;
  FarDialogItem* dlg_item = &buf;
#endif // FARAPI17
#ifdef FARAPI18
  Array<unsigned char> buf;
  unsigned size = static_cast<unsigned>(g_far.SendDlgMessage(h_dlg, DM_GETDLGITEM, index[ctrl_id], NULL));
  buf.extend(size).set_size(size);
  FarDialogItem* dlg_item = reinterpret_cast<FarDialogItem*>(buf.buf());
#endif // FARAPI18
  g_far.SendDlgMessage(h_dlg, DM_GETDLGITEM, index[ctrl_id], reinterpret_cast<LONG_PTR>(dlg_item));
  dlg_item->Flags &= ~DIF_COLORMASK;
  dlg_item->Flags |= DIF_SETCOLOR | color;
  g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, index[ctrl_id], reinterpret_cast<LONG_PTR>(dlg_item));
}

void FarDialog::set_focus(unsigned ctrl_id) {
  g_far.SendDlgMessage(h_dlg, DM_SETFOCUS, index[ctrl_id], 0);
}

void FarDialog::enable(unsigned ctrl_id, bool enable) {
  g_far.SendDlgMessage(h_dlg, DM_ENABLE, index[ctrl_id], enable ? TRUE : FALSE);
}

void FarDialog::link_label(unsigned ctrl_id, unsigned label_id) {
  assert(ctrl_id != label_id);
  unsigned label_idx = index[label_id];
  FarDialogItem label = items[label_idx];
  items.remove(label_idx);
  for (unsigned i = 0; i < index.size(); i++) {
    if (index[i] > label_idx) index.item(i)--;
  }
  unsigned ctrl_idx = index[ctrl_id];
  items.insert(ctrl_idx, label);
  for (unsigned i = 0; i < index.size(); i++) {
    if (index[i] >= ctrl_idx) index.item(i)++;
  }
  index.item(label_id) = ctrl_idx;
}
