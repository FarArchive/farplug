#ifndef _DLGAPI_H
#define _DLGAPI_H

#define CHANGE_FG(color, fg) ((fg) | (color) & ~FOREGROUND_INTENSITY & ~FOREGROUND_RED & ~FOREGROUND_GREEN & ~FOREGROUND_BLUE)
#define AUTO_SIZE (-1)

extern const unsigned c_x_frame;
extern const unsigned c_y_frame;
extern const wchar_t c_top_left;
extern const wchar_t c_top_right;
extern const wchar_t c_horiz1;
extern const wchar_t c_vert1;
extern const wchar_t c_horiz2;
extern const wchar_t c_vert2;
extern const wchar_t c_bottom_left;
extern const wchar_t c_bottom_right;
extern const wchar_t c_pb_black;
extern const wchar_t c_pb_white;

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
  ObjectArray<FarStr> values;
  struct ListData {
    Array<FarList> list;
    Array<FarListItem> items;
  };
  ObjectArray<ListData> lists;
  // set text for dialog element
  void set_text(FarDialogItem& di, const UnicodeString& text);
  // prepare buffer for EDIT controls
  void set_buf(FarDialogItem& di, const UnicodeString& text, unsigned bufsize);
  // set text for list items
  void set_list_text(FarListItem& li, const UnicodeString& text);
  void frame(const UnicodeString& text);
  void calc_frame_size();
public:
  FarDialog(const UnicodeString& title, unsigned width);
  FarDialogItem& item() {
    return items.last_item();
  }
  void new_line();
  void spacer(unsigned size);
  void pad(unsigned pos) {
    if (pos > x - c_x_frame) spacer(pos - (x - c_x_frame));
  }
  unsigned separator();
  unsigned label(const UnicodeString& text);
  unsigned var_edit_box(const UnicodeString& text, unsigned bufsize, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned mask_edit_box(const UnicodeString& text, const UnicodeString& mask, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned button(const UnicodeString& text, DWORD flags = 0, bool def = false);
  unsigned def_button(const UnicodeString& text, DWORD flags = 0) {
    return button(text, flags);
  }
  unsigned check_box(const UnicodeString& text, int value, DWORD flags = 0);
  unsigned check_box(const UnicodeString& text, bool value, DWORD flags = 0) {
    return check_box(text, value ? 1 : 0, flags);
  }
  unsigned radio_button(const UnicodeString& text, bool value, DWORD flags = 0);
  unsigned combo_box(const ObjectArray<UnicodeString>& items, unsigned sel_idx, unsigned bufsize, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  int show(FARWINDOWPROC dlg_proc = NULL, void* dlg_data = NULL, const FarCh* help = NULL);
};

#endif // _DLGAPI_H
