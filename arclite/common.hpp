#pragma once

typedef ByteVector ArcType;

typedef list<Error> ErrorLog;

#define RETRY_OR_IGNORE_BEGIN \
  bool error_ignored = false; \
  while (true) { \
    try {

#define RETRY_OR_IGNORE_END(ignore_errors, error_log, progress) \
      break; \
    } \
    catch (const Error& error) { \
      retry_or_ignore_error(error, error_ignored, ignore_errors, error_log, progress, true, true); \
      if (error_ignored) break; \
    } \
  }

#define RETRY_END(progress) \
      break; \
    } \
    catch (const Error& error) { \
      bool ignore_errors = false; \
      retry_or_ignore_error(error, error_ignored, ignore_errors, ErrorLog(), progress, true, false); \
    } \
  }

#define IGNORE_END(ignore_errors, error_log, progress) \
      break; \
    } \
    catch (const Error& error) { \
      retry_or_ignore_error(error, error_ignored, ignore_errors, error_log, progress, false, true); \
      if (error_ignored) break; \
    } \
  }

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
  bool paused;
  bool low_priority;
  DWORD initial_priority;
  bool confirm_esc;
  void update_time();
  void discard_time();
  void display();
protected:
  wstring progress_title;
  wstring progress_text;
  bool progress_known;
  unsigned percent_done;
  virtual void do_update_ui() = 0;
protected:
  bool is_single_key(const KEY_EVENT_RECORD& key_event);
  void handle_esc();
public:
  ProgressMonitor(const wstring& progress_title, bool progress_known = true, bool lazy = true);
  virtual ~ProgressMonitor();
  void update_ui(bool force = false);
  void clean();
  unsigned __int64 time_elapsed();
  unsigned __int64 ticks_per_sec();
  friend class ProgressSuspend;
};

class ProgressSuspend: private NonCopyable {
private:
  ProgressMonitor& progress;
public:
  ProgressSuspend::ProgressSuspend(ProgressMonitor& progress): progress(progress) {
    progress.update_time();
  }
  ProgressSuspend::~ProgressSuspend() {
    progress.discard_time();
  }
};

void retry_or_ignore_error(const Error& error, bool& ignore, bool& ignore_errors, ErrorLog& error_log, ProgressMonitor& progress, bool can_retry, bool can_ignore);

struct ExtractOptions {
  wstring dst_dir;
  bool ignore_errors;
  TriState overwrite;
  TriState move_files;
  wstring password;
  TriState separate_dir;
  bool delete_archive;
  ExtractOptions();
};

struct UpdateOptions {
  wstring arc_path;
  ArcType arc_type;
  unsigned level;
  wstring method;
  bool solid;
  wstring password;
  bool show_password;
  bool encrypt;
  TriState encrypt_header;
  bool create_sfx;
  wstring sfx_module;
  bool enable_volumes;
  wstring volume_size;
  bool move_files;
  bool open_shared;
  bool ignore_errors;
  wstring advanced;
  UpdateOptions();
};

struct UpdateProfile {
  wstring name;
  UpdateOptions options;
};
struct UpdateProfiles: public vector<UpdateProfile> {
  void load();
  void save() const;
};

struct Attr {
  wstring name;
  wstring value;
};
typedef list<Attr> AttrList;

unsigned calc_percent(unsigned __int64 completed, unsigned __int64 total);
unsigned __int64 get_module_version(const wstring& file_path);
unsigned __int64 parse_size_string(const wstring& str);
DWORD translate_seek_method(UInt32 seek_origin);
wstring expand_macros(const wstring& text);
void attach_sfx_module(const wstring& file_path, const wstring& sfx_module);
wstring load_file(const wstring& file_name, unsigned* code_page = nullptr);
