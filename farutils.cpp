#include <string>
#include <vector>
using namespace std;

#include "utils.hpp"
#include "farutils.hpp"
#include "options.hpp"
#include "ui.hpp"

namespace Far {

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

void init(const PluginStartupInfo *psi) {
  g_far = *psi;
  g_fsf = *psi->FSF;
}

wstring get_plugin_module_path() {
  return extract_file_path(g_far.ModuleName);
}

wstring get_root_key_name() {
  return g_far.RootKey;
}

unsigned get_version() {
  DWORD version;
  g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARVERSION, &version);
  return (LOWORD(version) << 16) | HIWORD(version);
}

const wchar_t* msg_ptr(int id) {
  return g_far.GetMsg(g_far.ModuleNumber, id);
}

wstring get_msg(int id) {
  return g_far.GetMsg(g_far.ModuleNumber, id);
}

unsigned get_optimal_msg_width() {
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
  if (con != INVALID_HANDLE_VALUE) {
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    if (GetConsoleScreenBufferInfo(con, &con_info)) {
      unsigned con_width = con_info.srWindow.Right - con_info.srWindow.Left + 1;
      if (con_width >= 80)
        return con_width - 20;
    }
  }
  return 60;
}

int message(const wstring& msg, int button_cnt, DWORD flags) {
  return g_far.Message(g_far.ModuleNumber, flags | FMSG_ALLINONE, NULL, reinterpret_cast<const wchar_t* const*>(msg.data()), 0, button_cnt);
}

void call_user_apc(void* param) {
  g_far.AdvControl(g_far.ModuleNumber, ACTL_SYNCHRO, param);
}

unsigned get_label_len(const wstring& str) {
  unsigned cnt = 0;
  for (unsigned i = 0; i < str.size(); i++) {
    if (str[i] != '&') cnt++;
  }
  return cnt;
}

void Dialog::set_text(FarDialogItem& di, const wstring& text) {
  values.push_back(text);
  di.PtrData = values.back().c_str();
}

void Dialog::set_list_text(FarListItem& li, const wstring& text) {
  values.push_back(text);
  li.Text = values.back().c_str();
}

void Dialog::frame(const wstring& text) {
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

void Dialog::calc_frame_size() {
  client_ys = y - c_y_frame;
  FarDialogItem& di = items.front(); // dialog frame
  di.X2 = c_x_frame + client_xs + 1;
  di.Y2 = c_y_frame + client_ys;
}

unsigned Dialog::new_item(const FarDialogItem& di) {
  items.push_back(di);
  return static_cast<unsigned>(items.size()) - 1;
}

LONG_PTR WINAPI Dialog::internal_dialog_proc(HANDLE h_dlg, int msg, int param1, LONG_PTR param2) {
  Dialog* dlg = reinterpret_cast<Dialog*>(g_far.SendDlgMessage(h_dlg, DM_GETDLGDATA, 0, 0));
  dlg->h_dlg = h_dlg;
  FAR_ERROR_HANDLER_BEGIN;
  return dlg->dialog_proc(msg, param1, param2);
  FAR_ERROR_HANDLER_END(;, ;, false);
  return dlg->default_dialog_proc(msg, param1, param2);
}

LONG_PTR Dialog::default_dialog_proc(int msg, int param1, LONG_PTR param2) {
  return g_far.DefDlgProc(h_dlg, msg, param1, param2);
}

Dialog::Dialog(const wstring& title, unsigned width, const wchar_t* help): client_xs(width), x(c_x_frame), y(c_y_frame), help(help) {
  frame(title);
}

void Dialog::new_line() {
  x = c_x_frame;
  y++;
}

void Dialog::spacer(unsigned size) {
  x += size;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
}

unsigned Dialog::separator() {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_TEXT;
  di.Y1 = y;
  di.Y2 = y;
  di.Flags = DIF_SEPARATOR;
  return new_item(di);
}

unsigned Dialog::label(const wstring& text, unsigned boxsize, DWORD flags) {
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

unsigned Dialog::edit_box(const wstring& text, unsigned boxsize, DWORD flags) {
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
  set_text(di, text);
  return new_item(di);
}

unsigned Dialog::mask_edit_box(const wstring& text, const wstring& mask, unsigned boxsize, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_FIXEDIT;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x += static_cast<unsigned>(mask.size());
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  values.push_back(mask);
  di.Mask = values.back().c_str();
  di.Flags = DIF_MASKEDIT | flags;
  set_text(di, text);
  return new_item(di);
}

unsigned Dialog::fix_edit_box(const wstring& text, unsigned boxsize, DWORD flags) {
  FarDialogItem di;
  memset(&di, 0, sizeof(di));
  di.Type = DI_FIXEDIT;
  di.X1 = x;
  di.Y1 = y;
  if (boxsize == AUTO_SIZE) x += static_cast<unsigned>(text.size());
  else x += boxsize;
  if (x - c_x_frame > client_xs) client_xs = x - c_x_frame;
  di.X2 = x - 1;
  di.Y2 = y;
  di.Flags = flags;
  set_text(di, text);
  return new_item(di);
}

unsigned Dialog::button(const wstring& text, DWORD flags, bool def) {
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

unsigned Dialog::check_box(const wstring& text, int value, DWORD flags) {
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

unsigned Dialog::radio_button(const wstring& text, bool value, DWORD flags) {
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

unsigned Dialog::combo_box(const vector<wstring>& list_items, unsigned sel_idx, unsigned boxsize, DWORD flags) {
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
    list_data.items.push_back(li);
  }
  FarList fl;
  fl.ItemsNumber = static_cast<DWORD>(list_data.items.size());
  fl.Items = const_cast<FarListItem*>(&list_data.items[0]);
  list_data.list.push_back(fl);
  lists.push_back(list_data);
  di.ListItems = const_cast<FarList*>(&lists.back().list[0]);
  set_text(di, wstring());
  return new_item(di);
}

int Dialog::show() {
  calc_frame_size();
  int res = -1;
  HANDLE h_dlg = g_far.DialogInit(g_far.ModuleNumber, -1, -1, client_xs + 2 * c_x_frame, client_ys + 2 * c_y_frame, help, &items[0], static_cast<unsigned>(items.size()), 0, 0, internal_dialog_proc, reinterpret_cast<LONG_PTR>(this));
  if (h_dlg != INVALID_HANDLE_VALUE) {
    res = g_far.DialogRun(h_dlg);
    g_far.DialogFree(h_dlg);
  }
  return res;
}

Dialog* Dialog::get_dlg(HANDLE h_dlg) {
  Dialog* dlg = reinterpret_cast<Dialog*>(g_far.SendDlgMessage(h_dlg, DM_GETDLGDATA, 0, 0));
  dlg->h_dlg = h_dlg;
  return dlg;
}

wstring Dialog::get_text(unsigned ctrl_id) {
  size_t len = g_far.SendDlgMessage(h_dlg, DM_GETTEXTLENGTH, ctrl_id, 0);
  Buffer<wchar_t> buf(len + 1);
  g_far.SendDlgMessage(h_dlg, DM_GETTEXTPTR, ctrl_id, reinterpret_cast<LONG_PTR>(buf.data()));
  return wstring(buf.data(), len);
}

void Dialog::set_text(unsigned ctrl_id, const wstring& text) {
  g_far.SendDlgMessage(h_dlg, DM_SETTEXTPTR, ctrl_id, reinterpret_cast<LONG_PTR>(text.c_str()));
}

bool Dialog::get_check(unsigned ctrl_id) {
  return DlgItem_GetCheck(g_far, h_dlg, ctrl_id) == BSTATE_CHECKED;
}

void Dialog::set_check(unsigned ctrl_id, bool check) {
  g_far.SendDlgMessage(h_dlg, DM_SETCHECK, ctrl_id, check ? BSTATE_CHECKED : BSTATE_UNCHECKED);
}

unsigned Dialog::get_list_pos(unsigned ctrl_id) {
  return static_cast<unsigned>(g_far.SendDlgMessage(h_dlg, DM_LISTGETCURPOS, ctrl_id, 0));
}

void Dialog::set_color(unsigned ctrl_id, unsigned char color) {
  size_t size = g_far.SendDlgMessage(h_dlg, DM_GETDLGITEM, ctrl_id, NULL);
  Buffer<unsigned char> buf(size);
  FarDialogItem* dlg_item = reinterpret_cast<FarDialogItem*>(buf.data());
  g_far.SendDlgMessage(h_dlg, DM_GETDLGITEM, ctrl_id, reinterpret_cast<LONG_PTR>(dlg_item));
  dlg_item->Flags &= ~DIF_COLORMASK;
  dlg_item->Flags |= DIF_SETCOLOR | color;
  g_far.SendDlgMessage(h_dlg, DM_SETDLGITEM, ctrl_id, reinterpret_cast<LONG_PTR>(dlg_item));
}

void Dialog::set_focus(unsigned ctrl_id) {
  g_far.SendDlgMessage(h_dlg, DM_SETFOCUS, ctrl_id, 0);
}

void Dialog::enable(unsigned ctrl_id, bool enable) {
  g_far.SendDlgMessage(h_dlg, DM_ENABLE, ctrl_id, enable ? TRUE : FALSE);
}

};
