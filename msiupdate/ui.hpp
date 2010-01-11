#pragma once

void error_dlg(const Error& e);
void error_dlg(const std::exception& e);
void error_dlg(const wstring& msg);
void info_dlg(const wstring& msg);

class ProgressMonitor {
private:
  HANDLE h_scr;
  wstring con_title;
  unsigned __int64 t_next;
  unsigned __int64 t_freq;
protected:
  virtual void do_update_ui() = 0;
public:
  ProgressMonitor(bool lazy = true);
  virtual ~ProgressMonitor();
  void update_ui(bool force = false);
};

bool config_dialog(Options& options);
