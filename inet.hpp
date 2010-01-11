#pragma once

class LoadUrlProgress: public ProgressMonitor, private CriticalSection {
private:
  unsigned total;
  unsigned done;
public:
  LoadUrlProgress(): ProgressMonitor(), total(0), done(0) {
  }
  virtual void do_update_ui();
  void set(unsigned done, unsigned total) {
    if (this) {
      CriticalSectionLock cs_lock(*this);
      this->done = done;
      this->total = total;
    }
  }
};

string load_url(const wstring& url, const HttpOptions& options, HANDLE h_abort, LoadUrlProgress* progress = NULL);
string load_url(const wstring& url, const HttpOptions& options);
