#pragma once

enum OverwriteOption {
  ooAsk = 0,
  ooOverwrite = 1,
  ooSkip = 2,
};

struct ExtractOptions {
  wstring dst_dir;
  bool ignore_errors;
  OverwriteOption overwrite;
  bool move_enabled;
  bool move_files;
  bool show_dialog;
  wstring password;
};

struct UpdateOptions {
  wstring arc_path;
  string arc_type;
  unsigned level;
  wstring method;
  bool solid;
  wstring password;
  bool show_password;
  bool encrypt;
  bool encrypt_header;
  bool encrypt_header_defined;
  bool create_sfx;
  unsigned sfx_module_idx;
  bool enable_volumes;
  wstring volume_size;
  bool move_files;
  bool open_shared;
  bool ignore_errors;
};

struct Attr {
  wstring name;
  wstring value;
};
typedef list<Attr> AttrList;

class ErrorLog: public map<wstring, Error> {
public:
  void add(const wstring& file_path, const Error& e) {
    if (count(file_path) == 0)
      (*this)[file_path] = e;
  }
};

class ProgressMonitor;
bool retry_or_ignore_error(const wstring& path, const Error& error, bool& ignore_errors, ErrorLog& error_log, ProgressMonitor& progress);
void ignore_error(const wstring& path, const Error& error, bool& ignore_errors, ErrorLog& error_log, ProgressMonitor& progress);

unsigned calc_percent(unsigned __int64 completed, unsigned __int64 total);
unsigned __int64 get_module_version(const wstring& file_path);
unsigned __int64 parse_size_string(const wstring& str);
DWORD translate_seek_method(UInt32 seek_origin);
