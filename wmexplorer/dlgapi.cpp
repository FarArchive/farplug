#define _ERROR_WINDOWS
#include "error.h"
#include "guids.h"
#include "util.h"
#include "dlgapi.h"

extern struct PluginStartupInfo g_far;
extern Array<unsigned char> g_colors;

// dialog frame size
const unsigned c_x_frame = 5;
const unsigned c_y_frame = 2;

// progress bar chars
const wchar_t c_pb_black = 9608;
const wchar_t c_pb_white = 9617;

unsigned get_label_len(const UnicodeString& str) {
  unsigned cnt = 0;
  for (unsigned i = 0; i < str.size(); i++) {
    if (str[i] != '&') cnt++;
  }
  return cnt;
}

void FarDialog::set_text(FarDialogItem& di, const UnicodeString& text) {
  values += text;
  di.Data = values.last().data();
}

void FarDialog::set_buf(FarDialogItem& di, const UnicodeString& text) {
  set_text(di, text);
}

void FarDialog::set_list_text(FarListItem& li, const UnicodeString& text) {
  values += text;
  li.Text = values.last().data();
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
  items += di;
}

void FarDialog::calc_frame_size() {
  client_ys = y - c_y_frame;
  FarDialogItem& di = items.item(0); // dialog frame
  di.X2 = c_x_frame + client_xs + 1;
  di.Y2 = c_y_frame + client_ys;
}

FarDialog::FarDialog(const GUID& guid, const UnicodeString& title, unsigned width): guid(guid), client_xs(width), x(c_x_frame), y(c_y_frame) {
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

void FarDialog::pad(unsigned pos) {
  if (pos > x - c_x_frame) spacer(pos - (x - c_x_frame));
}

unsigned FarDialog::separator() {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.Y1 = y;
  di.Y2 = y;
  di.Flags = DIF_SEPARATOR;
  items += di;
  return items.size() - 1;
}

unsigned FarDialog::label(const UnicodeString& text) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.X1 = x;
  di.Y1 = y;
  x += get_label_len(text);
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  set_text(di, text);
  items += di;
  return items.size() - 1;
}

unsigned FarDialog::var_edit_box(const UnicodeString& text, unsigned boxsize, DWORD flags) {
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
  di.Flags = flags;
  set_buf(di, text);
  items += di;
  return items.size() - 1;
}

unsigned FarDialog::mask_edit_box(const UnicodeString& text, const UnicodeString& mask, unsigned boxsize, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_FIXEDIT;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x = c_x_frame + client_xs;
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  values += mask;
  di.Mask = values.last().data();
  di.Flags = DIF_MASKEDIT | flags;
  set_buf(di, text);
  items += di;
  return items.size() - 1;
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
  di.Flags = flags | (def ? DIF_DEFAULTBUTTON : 0);
  set_text(di, text);
  items += di;
  return items.size() - 1;
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
  items += di;
  return items.size() - 1;
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
  items += di;
  return items.size() - 1;
}

unsigned FarDialog::combo_box(const ObjectArray<UnicodeString>& list_items, unsigned sel_idx, unsigned boxsize, DWORD flags) {
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
  di.Flags = flags;
  ListData list_data;
  for (unsigned i = 0; i < list_items.size(); i++) {
    FarListItem li;
    memset(&li, 0, sizeof(li));
    set_list_text(li, list_items[i]);
    if (i == sel_idx) li.Flags = LIF_SELECTED;
    list_data.items += li;
  }
  FarList fl = { sizeof(FarList) };
  fl.ItemsNumber = list_data.items.size();
  fl.Items = const_cast<FarListItem*>(list_data.items.data());
  list_data.list = fl;
  lists += list_data;
  di.ListItems = const_cast<FarList*>(lists.last().list.data());
  set_buf(di, L"");
  items += di;
  return items.size() - 1;
}

intptr_t FarDialog::show(FARWINDOWPROC dlg_proc, void* dlg_data, const wchar_t* help) {
  calc_frame_size();
  intptr_t res = -1;
  HANDLE h_dlg = g_far.DialogInit(&c_plugin_guid, &guid, -1, -1, client_xs + 2 * c_x_frame, client_ys + 2 * c_y_frame, help, items.buf(), items.size(), 0, 0, dlg_proc, dlg_data);
  if (h_dlg != INVALID_HANDLE_VALUE) {
    res = g_far.DialogRun(h_dlg);
    g_far.DialogFree(h_dlg);
  }
  return res;
}
