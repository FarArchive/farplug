#pragma once

class TrayIcon: public MessageWindow, public Icon {
private:
  static const unsigned c_timeout = 30 * 1000;
  NOTIFYICONDATAW nid;
  bool win2k;
protected:
  virtual LRESULT window_proc(UINT msg, WPARAM w_param, LPARAM l_param);
public:
  TrayIcon(const wstring& window_name, WORD icon_id, const wstring& title, const wstring& text);
  virtual ~TrayIcon();
};
