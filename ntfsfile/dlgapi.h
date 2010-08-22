#ifndef _DLGAPI_H
#define _DLGAPI_H

#define CHANGE_FG(color, fg) ((fg) | (color) & ~FOREGROUND_INTENSITY & ~FOREGROUND_RED & ~FOREGROUND_GREEN & ~FOREGROUND_BLUE)
#define AUTO_SIZE (-1)

extern const unsigned c_x_frame;
extern const unsigned c_y_frame;
extern const wchar_t c_horiz1;
extern const wchar_t c_vert1;
extern const wchar_t c_horiz2;
extern const wchar_t c_vert2;
extern const wchar_t c_top2_left2;
extern const wchar_t c_top2_right2;
extern const wchar_t c_bottom2_left2;
extern const wchar_t c_bottom2_right2;
extern const wchar_t c_left2_horiz1;
extern const wchar_t c_right2_horiz1;
extern const wchar_t c_top1_vert1;
extern const wchar_t c_bottom1_vert1;
extern const wchar_t c_bottom2_vert1;
extern const wchar_t c_cross1;

extern const wchar_t c_pb_black;
extern const wchar_t c_pb_white;

UnicodeString far_get_msg(int id);
const FarCh* far_msg_ptr(int id);
unsigned get_msg_width();
int far_message(const UnicodeString& msg, int button_cnt = 0, DWORD flags = 0);
int far_menu(const UnicodeString& title, const ObjectArray<UnicodeString>& items, const FarCh* help = NULL);
int far_viewer(const UnicodeString& file_name, const UnicodeString& title);

unsigned get_label_len(const UnicodeString& str);
unsigned get_fmt_str_len(const UnicodeString& str);
void draw_fmt_str(unsigned x, unsigned y, const UnicodeString& str, unsigned xs = -1);
void draw_frame(unsigned x0, unsigned y0, unsigned w, unsigned h, const UnicodeString& title);
void draw_text_box(const UnicodeString& title, const ObjectArray<UnicodeString>& lines, unsigned client_xs);

class FarDialog {
private:
  unsigned client_xs;
  unsigned client_ys;
  unsigned x;
  unsigned y;
  Array<FarDialogItem> items;
  Array<unsigned> index;
  ObjectArray<FarStr> values;
  struct ListData {
    Array<FarList> list;
    Array<FarListItem> items;
  };
  ObjectArray<ListData> lists;
  HANDLE h_dlg;
  Array<void*> dlg_data;
  // set text for dialog element
  void set_text(FarDialogItem& di, const UnicodeString& text);
  // prepare buffer for EDIT controls
  void set_buf(FarDialogItem& di, const UnicodeString& text, unsigned bufsize);
  // set text for list items
  void set_list_text(FarListItem& li, const UnicodeString& text);
  void frame(const UnicodeString& text);
  void calc_frame_size();
  unsigned new_item(const FarDialogItem& di);
public:
  FarDialog(const UnicodeString& title, unsigned width);
  // create different controls
  void new_line();
  void spacer(unsigned size);
  void pad(unsigned pos) {
    if (pos > x - c_x_frame) spacer(pos - (x - c_x_frame));
  }
  unsigned separator();
  unsigned separator(const UnicodeString& text);
  unsigned label(const UnicodeString& text, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned var_edit_box(const UnicodeString& text, unsigned bufsize, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned mask_edit_box(const UnicodeString& text, const UnicodeString& mask, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned fix_edit_box(const UnicodeString& text, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned button(const UnicodeString& text, DWORD flags = 0, bool def = false);
  unsigned def_button(const UnicodeString& text, DWORD flags = 0) {
    return button(text, flags, true);
  }
  unsigned check_box(const UnicodeString& text, int value, DWORD flags = 0);
  unsigned check_box(const UnicodeString& text, bool value, DWORD flags = 0) {
    return check_box(text, value ? 1 : 0, flags);
  }
  unsigned radio_button(const UnicodeString& text, bool value, DWORD flags = 0);
  unsigned combo_box(const ObjectArray<UnicodeString>& items, unsigned sel_idx, unsigned bufsize, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  // display dialog
  int show(FARWINDOWPROC dlg_proc = NULL, const FarCh* help = NULL);
  // utilities to set/get control values
  static FarDialog* get_dlg(HANDLE h_dlg);
  void add_dlg_data(void* data) {
    dlg_data += data;
  }
  void* get_dlg_data(unsigned idx) {
    return dlg_data[idx];
  }
  UnicodeString get_text(unsigned ctrl_id);
  void set_text(unsigned ctrl_id, const UnicodeString& text);
  bool get_check(unsigned ctrl_id);
  void set_check(unsigned ctrl_id, bool check);
  unsigned get_list_pos(unsigned ctrl_id);
  void set_color(unsigned ctrl_id, unsigned char color);
  void set_focus(unsigned ctrl_id);
  void enable(unsigned ctrl_id, bool enable);
  void link_label(unsigned ctrl_id, unsigned label_id);
};

#endif // _DLGAPI_H
