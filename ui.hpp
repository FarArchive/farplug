#pragma once

class ProgressMonitor: private NonCopyable {
private:
  HANDLE h_scr;
  wstring con_title;
  static const unsigned c_first_delay_div = 2;
  static const unsigned c_update_delay_div = 2;
  unsigned __int64 time_cnt;
  unsigned __int64 time_freq;
  unsigned __int64 time_total;
  unsigned __int64 time_update;
protected:
  unsigned __int64 time_elapsed() const {
    return time_total / time_freq;
  }
  virtual void do_update_ui() = 0;
public:
  ProgressMonitor(bool lazy = true);
  virtual ~ProgressMonitor();
  void update_ui(bool force = false);
  friend class ProgressSuspend;
};

class ProgressSuspend: private NonCopyable {
private:
  ProgressMonitor& pm;
public:
  ProgressSuspend(ProgressMonitor& pm);
  ~ProgressSuspend();
};

const wchar_t** get_size_suffixes();
const wchar_t** get_speed_suffixes();

bool password_dialog(wstring& password);
