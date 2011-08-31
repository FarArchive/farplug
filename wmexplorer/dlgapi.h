#ifndef _DLGAPI_H
#define _DLGAPI_H

#define AUTO_SIZE (-1)

extern const wchar_t c_pb_black;
extern const wchar_t c_pb_white;

unsigned get_label_len(const UnicodeString& str);

class FarDialog {
private:
  const GUID& guid;
  unsigned client_xs;
  unsigned client_ys;
  unsigned x;
  unsigned y;
  Array<FarDialogItem> items;
  ObjectArray<UnicodeString> values;
  struct ListData {
    Array<FarList> list;
    Array<FarListItem> items;
  };
  ObjectArray<ListData> lists;
  // set text for dialog element
  void set_text(FarDialogItem& di, const UnicodeString& text);
  // prepare buffer for EDIT controls
  void set_buf(FarDialogItem& di, const UnicodeString& text);
  // set text for list items
  void set_list_text(FarListItem& li, const UnicodeString& text);
  void frame(const UnicodeString& text);
  void calc_frame_size();
public:
  FarDialog(const GUID& guid, const UnicodeString& title, unsigned width);
  FarDialogItem& item() {
    return items.last_item();
  }
  void new_line();
  void spacer(unsigned size);
  void pad(unsigned pos);
  unsigned separator();
  unsigned label(const UnicodeString& text);
  unsigned var_edit_box(const UnicodeString& text, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned mask_edit_box(const UnicodeString& text, const UnicodeString& mask, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  unsigned button(const UnicodeString& text, DWORD flags = 0, bool def = false);
  unsigned def_button(const UnicodeString& text, DWORD flags = 0) {
    return button(text, flags, true);
  }
  unsigned check_box(const UnicodeString& text, int value, DWORD flags = 0);
  unsigned check_box(const UnicodeString& text, bool value, DWORD flags = 0) {
    return check_box(text, value ? 1 : 0, flags);
  }
  unsigned radio_button(const UnicodeString& text, bool value, DWORD flags = 0);
  unsigned combo_box(const ObjectArray<UnicodeString>& items, unsigned sel_idx, unsigned boxsize = AUTO_SIZE, DWORD flags = 0);
  int show(FARWINDOWPROC dlg_proc = NULL, void* dlg_data = NULL, const wchar_t* help = NULL);
};

#endif // _DLGAPI_H
