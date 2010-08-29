#include "msg.h"

#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "common.hpp"
#include "ui.hpp"
#include "archive.hpp"

wstring get_error_dlg_title() {
  return Far::get_msg(MSG_PLUGIN_NAME);
}

ProgressMonitor::ProgressMonitor(bool lazy): h_scr(nullptr) {
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&time_cnt));
  QueryPerformanceFrequency(reinterpret_cast<PLARGE_INTEGER>(&time_freq));
  time_total = 0;
  if (lazy)
    time_update = time_total + time_freq / c_first_delay_div;
  else
    time_update = time_total;
  confirm_esc = (Far::adv_control(ACTL_GETCONFIRMATIONS) & FCS_INTERRUPTOPERATION) != 0;
}

ProgressMonitor::~ProgressMonitor() {
  if (h_scr) {
    Far::restore_screen(h_scr);
    SetConsoleTitleW(con_title.data());
    Far::set_progress_state(TBPF_NOPROGRESS);
  }
}

void ProgressMonitor::update_ui(bool force) {
  update_time();
  if ((time_total >= time_update) || force) {
    time_update = time_total + time_freq / c_update_delay_div;
    if (h_scr == nullptr) {
      h_scr = Far::save_screen();
      con_title = get_console_title();
    }
    HANDLE h_con = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD rec;
    DWORD read_cnt;
    while (true) {
      PeekConsoleInput(h_con, &rec, 1, &read_cnt);
      if (read_cnt == 0) break;
      ReadConsoleInput(h_con, &rec, 1, &read_cnt);
      if ((rec.EventType == KEY_EVENT) && (rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) && rec.Event.KeyEvent.bKeyDown && ((rec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED | SHIFT_PRESSED)) == 0)) {
        if (!confirm_esc)
          FAIL(E_ABORT);
        ProgressSuspend ps(*this);
        if (Far::message(Far::get_msg(MSG_PLUGIN_NAME) + L"\n" + Far::get_msg(MSG_PROGRESS_INTERRUPT), 0, FMSG_MB_YESNO) == 0)
          FAIL(E_ABORT);
      }
    }
    do_update_ui();
  }
}

void ProgressMonitor::update_time() {
  unsigned __int64 time_curr;
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&time_curr));
  time_total += time_curr - time_cnt;
  time_cnt = time_curr;
}

void ProgressMonitor::discard_time() {
  QueryPerformanceCounter(reinterpret_cast<PLARGE_INTEGER>(&time_cnt));
}

unsigned __int64 ProgressMonitor::time_elapsed() {
  update_time();
  return time_total;
}

unsigned __int64 ProgressMonitor::ticks_per_sec() {
  return time_freq;
}

const wchar_t** get_size_suffixes() {
  static const wchar_t* suffixes[5] = {
    L"",
    Far::msg_ptr(MSG_SUFFIX_SIZE_KB),
    Far::msg_ptr(MSG_SUFFIX_SIZE_MB),
    Far::msg_ptr(MSG_SUFFIX_SIZE_GB),
    Far::msg_ptr(MSG_SUFFIX_SIZE_TB),
  };
  return suffixes;
}

const wchar_t** get_speed_suffixes() {
  static const wchar_t* suffixes[5] = {
    Far::msg_ptr(MSG_SUFFIX_SPEED_B),
    Far::msg_ptr(MSG_SUFFIX_SPEED_KB),
    Far::msg_ptr(MSG_SUFFIX_SPEED_MB),
    Far::msg_ptr(MSG_SUFFIX_SPEED_GB),
    Far::msg_ptr(MSG_SUFFIX_SPEED_TB),
  };
  return suffixes;
}

class PasswordDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 40
  };

  wstring& password;

  int password_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
      password = get_text(password_ctrl_id);
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  PasswordDialog(wstring& password): Far::Dialog(Far::get_msg(MSG_PASSWORD_TITLE), c_client_xs), password(password) {
  }

  bool show() {
    label(Far::get_msg(MSG_PASSWORD_PASSWORD));
    password_ctrl_id = pwd_edit_box(password);
    new_line();
    separator();
    new_line();

    ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    return (item != -1) && (item != cancel_ctrl_id);
  }
};

bool password_dialog(wstring& password) {
  return PasswordDialog(password).show();
}

class OverwriteDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 60
  };

  int yes_ctrl_id;
  int yes_all_ctrl_id;
  int no_ctrl_id;
  int no_all_ctrl_id;
  int cancel_ctrl_id;

public:
  OverwriteDialog(): Far::Dialog(Far::get_msg(MSG_OVERWRITE_DLG_TITLE), c_client_xs) {
  }

  OverwriteAction show(const wstring& file_path, const FindData& src_file_info, const FindData& dst_file_info) {
    label(fit_str(file_path, c_client_xs));
    new_line();
    label(Far::get_msg(MSG_OVERWRITE_DLG_QUESTION));
    new_line();
    separator();
    new_line();

    label(Far::get_msg(MSG_OVERWRITE_DLG_SOURCE));
    pad(15);
    if (!src_file_info.is_dir()) {
      label(format_data_size(src_file_info.size(), get_size_suffixes()));
      pad(25);
    }
    label(format_file_time(src_file_info.ftLastWriteTime));
    if (CompareFileTime(&src_file_info.ftLastWriteTime, &dst_file_info.ftLastWriteTime) > 0) {
      spacer(1);
      label(Far::get_msg(MSG_OVERWRITE_DLG_NEWER));
    }
    new_line();

    label(Far::get_msg(MSG_OVERWRITE_DLG_DESTINATION));
    pad(15);
    if (!dst_file_info.is_dir()) {
      label(format_data_size(dst_file_info.size(), get_size_suffixes()));
      pad(25);
    }
    label(format_file_time(dst_file_info.ftLastWriteTime));
    if (CompareFileTime(&src_file_info.ftLastWriteTime, &dst_file_info.ftLastWriteTime) < 0) {
      spacer(1);
      label(Far::get_msg(MSG_OVERWRITE_DLG_NEWER));
    }
    new_line();

    separator();
    new_line();
    yes_ctrl_id = def_button(Far::get_msg(MSG_OVERWRITE_DLG_YES), DIF_CENTERGROUP);
    yes_all_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_YES_ALL), DIF_CENTERGROUP);
    no_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_NO), DIF_CENTERGROUP);
    no_all_ctrl_id = button(Far::get_msg(MSG_OVERWRITE_DLG_NO_ALL), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    if (item == yes_ctrl_id) return oaYes;
    else if (item == yes_all_ctrl_id) return oaYesAll;
    else if (item == no_ctrl_id) return oaNo;
    else if (item == no_all_ctrl_id) return oaNoAll;
    else return oaCancel;
  }
};

OverwriteAction overwrite_dialog(const wstring& file_path, const FindData& src_file_info, const FindData& dst_file_info) {
  return OverwriteDialog().show(file_path, src_file_info, dst_file_info);
}

class ExtractDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 60
  };

  ExtractOptions& options;

  int dst_dir_ctrl_id;
  int ignore_errors_ctrl_id;
  int oo_ask_ctrl_id;
  int oo_overwrite_ctrl_id;
  int oo_skip_ctrl_id;
  int move_files_ctrl_id;
  int password_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
      options.dst_dir = get_text(dst_dir_ctrl_id);
      options.dst_dir = unquote(strip(options.dst_dir));
      options.ignore_errors = get_check(ignore_errors_ctrl_id);
      if (get_check(oo_ask_ctrl_id)) options.overwrite = ooAsk;
      else if (get_check(oo_overwrite_ctrl_id)) options.overwrite = ooOverwrite;
      else options.overwrite = ooSkip;
      options.move_files = get_check(move_files_ctrl_id);
      options.password = get_text(password_ctrl_id);
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  ExtractDialog(ExtractOptions& options): Far::Dialog(Far::get_msg(MSG_EXTRACT_DLG_TITLE), c_client_xs), options(options) {
  }

  bool show() {
    label(Far::get_msg(MSG_EXTRACT_DLG_DST_DIR));
    new_line();
    dst_dir_ctrl_id = history_edit_box(options.dst_dir, L"arclite.extract_dir", c_client_xs, DIF_EDITPATH);
    new_line();
    separator();
    new_line();

    ignore_errors_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_IGNORE_ERRORS), options.ignore_errors);
    new_line();

    label(Far::get_msg(MSG_EXTRACT_DLG_OO));
    new_line();
    spacer(2);
    oo_ask_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OO_ASK), options.overwrite == ooAsk);
    spacer(2);
    oo_overwrite_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OO_OVERWRITE), options.overwrite == ooOverwrite);
    spacer(2);
    oo_skip_ctrl_id = radio_button(Far::get_msg(MSG_EXTRACT_DLG_OO_SKIP), options.overwrite == ooSkip);
    new_line();

    move_files_ctrl_id = check_box(Far::get_msg(MSG_EXTRACT_DLG_MOVE_FILES), options.move_files, options.move_enabled ? 0 : DIF_DISABLE);
    new_line();

    label(Far::get_msg(MSG_EXTRACT_DLG_PASSWORD));
    password_ctrl_id = pwd_edit_box(options.password, 20);
    new_line();

    separator();
    new_line();
    ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    return (item != -1) && (item != cancel_ctrl_id);
  }
};

bool extract_dialog(ExtractOptions& options) {
  return ExtractDialog(options).show();
}

RetryDialogResult error_retry_ignore_dialog(const wstring& file_path, const Error& e, bool can_retry) {
  wostringstream st;
  st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
  st << fit_str(file_path, Far::get_optimal_msg_width()) << L'\n';
  if (e.code != E_MESSAGE) {
    wstring sys_msg = get_system_message(e.code);
    if (!sys_msg.empty())
      st << word_wrap(sys_msg, Far::get_optimal_msg_width()) << L'\n';
  }
  for (list<wstring>::const_iterator msg = e.messages.begin(); msg != e.messages.end(); msg++) {
    st << word_wrap(*msg, Far::get_optimal_msg_width()) << L'\n';
  }
  st << extract_file_name(widen(e.file)) << L':' << e.line << L'\n';
  if (can_retry)
    st << Far::get_msg(MSG_BUTTON_RETRY) << L'\n';
  st << Far::get_msg(MSG_BUTTON_IGNORE) << L'\n';
  st << Far::get_msg(MSG_BUTTON_IGNORE_ALL) << L'\n';
  st << Far::get_msg(MSG_BUTTON_CANCEL) << L'\n';
  switch (Far::message(st.str(), can_retry ? 4 : 3, FMSG_WARNING)) {
  case 0:
    return can_retry ? rdrRetry : rdrIgnore;
  case 1:
    return can_retry ? rdrIgnore : rdrIgnoreAll;
  case 2:
    return can_retry ? rdrIgnoreAll : rdrCancel;
  default:
    return rdrCancel;
  }
}

void show_error_log(const ErrorLog& error_log) {
  wstring msg;
  msg += Far::get_msg(MSG_LOG_TITLE) + L'\n';
  msg += Far::get_msg(MSG_LOG_INFO) + L'\n';
  msg += Far::get_msg(MSG_LOG_CLOSE) + L'\n';
  msg += Far::get_msg(MSG_LOG_SHOW) + L'\n';
  if (Far::message(msg, 2, FMSG_WARNING) != 1) return;

  TempFile temp_file;
  File file(temp_file.get_path(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);

  const wchar_t sig = 0xFEFF;
  file.write(&sig, sizeof(sig));
  wstring line;
  for (ErrorLog::const_iterator iter = error_log.begin(); iter != error_log.end(); iter++) {
    const wstring& file_path = iter->first;
    const Error& error = iter->second;
    line.assign(file_path).append(1, L'\n');
    if (error.code != E_MESSAGE) {
      wstring sys_msg = get_system_message(error.code);
      if (!sys_msg.empty())
        line.append(sys_msg).append(1, L'\n');
    }
    for (list<wstring>::const_iterator err_msg = error.messages.begin(); err_msg != error.messages.end(); err_msg++) {
      line.append(*err_msg).append(1, L'\n');
    }
    line.append(1, L'\n');
    file.write(line.data(), line.size() * sizeof(wchar_t));
  }

  Far::viewer(temp_file.get_path(), Far::get_msg(MSG_LOG_TITLE));
}

struct ArchiveType {
  unsigned name_id;
  const string& value;
};

const ArchiveType c_archive_types[] = {
  { MSG_COMPRESSION_ARCHIVE_7Z, c_guid_7z },
  { MSG_COMPRESSION_ARCHIVE_ZIP, c_guid_zip },
};

struct CompressionLevel {
  unsigned name_id;
  unsigned value;
};

const CompressionLevel c_levels[] = {
  { MSG_COMPRESSION_LEVEL_STORE, 0 },
  { MSG_COMPRESSION_LEVEL_FASTEST, 1 },
  { MSG_COMPRESSION_LEVEL_FAST, 3 },
  { MSG_COMPRESSION_LEVEL_NORMAL, 5 },
  { MSG_COMPRESSION_LEVEL_MAXIMUM, 7 },
  { MSG_COMPRESSION_LEVEL_ULTRA, 9 },
};

struct CompressionMethod {
  unsigned name_id;
  const wchar_t* value;
};

const CompressionMethod c_methods[] = {
  { MSG_COMPRESSION_METHOD_LZMA, L"LZMA" },
  { MSG_COMPRESSION_METHOD_LZMA2, L"LZMA2" },
  { MSG_COMPRESSION_METHOD_PPMD, L"PPMD" },
};

class UpdateDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 65
  };

  bool new_arc;
  vector<ArcType> main_formats;
  vector<ArcType> other_formats;
  UpdateOptions& options;

  int arc_path_ctrl_id;
  int main_formats_ctrl_id;
  int other_formats_ctrl_id;
  int level_ctrl_id;
  int method_ctrl_id;
  int solid_ctrl_id;
  int encrypt_ctrl_id;
  int encrypt_header_ctrl_id;
  int password_ctrl_id;
  int password2_ctrl_id;
  int create_sfx_ctrl_id;
  int sfx_module_ctrl_id;
  int move_files_ctrl_id;
  int open_shared_ctrl_id;
  int ignore_errors_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  wstring old_ext;
  ArcType arc_type;

  bool change_extension() {
    assert(new_arc);

    wstring new_ext;
    bool create_sfx = get_check(create_sfx_ctrl_id);
    if (create_sfx && arc_type == c_guid_7z)
      new_ext = L".exe";
    else if (ArcAPI::formats().count(arc_type))
      new_ext = ArcAPI::formats().at(arc_type).default_extension();

    if (old_ext.empty() || new_ext.empty())
      return false;

    wstring arc_path = get_text(arc_path_ctrl_id);
    wstring file_name = extract_file_name(strip(unquote(strip(arc_path))));
    size_t pos = file_name.find_last_of(L'.');
    if (pos == wstring::npos)
      return false;
    wstring ext = file_name.substr(pos);
    if (_wcsicmp(old_ext.c_str(), ext.c_str()) != 0)
      return false;
    pos = arc_path.find_last_of(ext) - (ext.size() - 1);
    arc_path.replace(pos, ext.size(), new_ext);
    set_text(arc_path_ctrl_id, arc_path);

    old_ext = new_ext;
    return true;
  }

  void set_control_state() {
    bool is_7z = arc_type == c_guid_7z;
    for (int i = method_ctrl_id - 1; i < method_ctrl_id + static_cast<int>(ARRAYSIZE(c_methods)); i++) {
      enable(i, is_7z);
    }
    enable(solid_ctrl_id, is_7z);
    bool other_format = get_check(other_formats_ctrl_id);
    enable(encrypt_ctrl_id, !other_format);
    bool encrypt = get_check(encrypt_ctrl_id);
    for (int i = encrypt_ctrl_id + 1; i <= password2_ctrl_id; i++) {
      enable(i, encrypt && !other_format);
    }
    enable(encrypt_header_ctrl_id, is_7z && encrypt);
    if (new_arc) {
      change_extension();
      enable(create_sfx_ctrl_id, is_7z);
      bool create_sfx = get_check(create_sfx_ctrl_id);
      for (int i = create_sfx_ctrl_id + 1; i <= sfx_module_ctrl_id; i++) {
        enable(i, is_7z && create_sfx);
      }
      enable(other_formats_ctrl_id + 1, other_format);
    }
  }

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if (msg == DN_CLOSE && param1 >= 0 && param1 != cancel_ctrl_id) {
      if (new_arc) {
        options.arc_path = unquote(strip(get_text(arc_path_ctrl_id)));

        ArcType arc_type;
        for (unsigned i = 0; i < main_formats.size(); i++) {
          if (get_check(main_formats_ctrl_id + i)) {
            arc_type = c_archive_types[i].value;
            break;
          }
        }
        if (!other_formats.empty() && get_check(other_formats_ctrl_id)) {
          arc_type = other_formats[get_list_pos(other_formats_ctrl_id + 1)];
        }
        if (arc_type.empty()) {
          FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_WRONG_ARC_TYPE));
        }
        options.arc_type = arc_type;
      }
      for (unsigned i = 0; i < ARRAYSIZE(c_levels); i++) {
        if (get_check(level_ctrl_id + i)) {
          options.level = c_levels[i].value;
          break;
        }
      }
      for (unsigned i = 0; i < ARRAYSIZE(c_methods); i++) {
        if (get_check(method_ctrl_id + i)) {
          options.method = c_methods[i].value;
          break;
        }
      }
      options.solid = get_check(solid_ctrl_id);
      if (get_check(encrypt_ctrl_id)) {
        wstring password = get_text(password_ctrl_id);
        wstring password2 = get_text(password2_ctrl_id);
        if (password != password2) {
          FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_PASSWORDS_DONT_MATCH));
        }
        if (password.empty()) {
          FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_PASSWORD_IS_EMPTY));
        }
        options.password = password;
        options.encrypt_header = get_check(encrypt_header_ctrl_id);
      }
      else {
        options.password.clear();
      }
      if (new_arc) {
        options.create_sfx = get_check(create_sfx_ctrl_id);
        if (options.create_sfx) {
          options.sfx_module_idx = get_list_pos(sfx_module_ctrl_id);
          if (options.sfx_module_idx >= ArcAPI::sfx().size()) {
            FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_WRONG_SFX_MODULE));
          }
        }
      }
      options.move_files = get_check(move_files_ctrl_id);
      options.open_shared = get_check(open_shared_ctrl_id);
      options.ignore_errors = get_check(ignore_errors_ctrl_id);
    }
    else if (msg == DN_INITDIALOG) {
      set_control_state();
    }
    else if (new_arc && msg == DN_BTNCLICK && !main_formats.empty() && param1 >= main_formats_ctrl_id && param1 < main_formats_ctrl_id + static_cast<int>(main_formats.size())) {
      if (param2) {
        arc_type = main_formats[param1 - main_formats_ctrl_id];
        set_control_state();
      }
    }
    else if (new_arc && msg == DN_BTNCLICK && !other_formats.empty() && param1 == other_formats_ctrl_id) {
      if (param2) {
        arc_type = other_formats[get_list_pos(other_formats_ctrl_id + 1)];
        set_control_state();
      }
    }
    else if (new_arc && msg == DN_EDITCHANGE && !other_formats.empty() && param1 == other_formats_ctrl_id + 1) {
      arc_type = other_formats[get_list_pos(other_formats_ctrl_id + 1)];
      set_control_state();
    }
    else if (msg == DN_BTNCLICK && param1 == encrypt_ctrl_id) {
      set_control_state();
    }
    else if (new_arc && msg == DN_BTNCLICK && param1 == create_sfx_ctrl_id) {
      set_control_state();
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  UpdateDialog(bool new_arc, UpdateOptions& options): Far::Dialog(Far::get_msg(new_arc ? MSG_UPDATE_DLG_TITLE_CREATE : MSG_UPDATE_DLG_TITLE), c_client_xs), new_arc(new_arc), options(options), arc_type(options.arc_type) {
  }

  bool show() {
    if (new_arc) {
      old_ext = extract_file_ext(options.arc_path);

      label(Far::get_msg(MSG_UPDATE_DLG_ARC_PATH));
      new_line();
      arc_path_ctrl_id = history_edit_box(options.arc_path, L"arclite.arc_path", c_client_xs, DIF_EDITPATH);
      new_line();
      separator();
      new_line();

      label(Far::get_msg(MSG_UPDATE_DLG_ARC_TYPE));
      new_line();

      const ArcFormats& arc_formats = ArcAPI::formats();
      for (unsigned i = 0; i < ARRAYSIZE(c_archive_types); i++) {
        ArcFormats::const_iterator arc_iter = arc_formats.find(c_archive_types[i].value);
        if (arc_iter != arc_formats.end() && arc_iter->second.updatable) {
          bool first = main_formats.size() == 0;
          if (!first)
            spacer(1);
          int ctrl_id = radio_button(Far::get_msg(c_archive_types[i].name_id), options.arc_type == c_archive_types[i].value, first ? DIF_GROUP : 0);
          if (first)
            main_formats_ctrl_id = ctrl_id;
          main_formats.push_back(c_archive_types[i].value);
        }
      };

      vector<wstring> format_names;
      unsigned other_format_index = 0;
      bool found = false;
      for (ArcFormats::const_iterator arc_iter = arc_formats.begin(); arc_iter != arc_formats.end(); arc_iter++) {
        if (arc_iter->second.updatable) {
          vector<ArcType>::const_iterator main_type = find(main_formats.begin(), main_formats.end(), arc_iter->first);
          if (main_type == main_formats.end()) {
            other_formats.push_back(arc_iter->first);
            format_names.push_back(arc_iter->second.name);
            if (options.arc_type == arc_iter->first) {
              other_format_index = other_formats.size() - 1;
              found = true;
            }
          }
        }
      }
      if (!other_formats.empty()) {
        if (!main_formats.empty())
          spacer(1);
        other_formats_ctrl_id = radio_button(Far::get_msg(MSG_UPDATE_DLG_ARC_TYPE_OTHER), found);
        combo_box(format_names, other_format_index, AUTO_SIZE, DIF_DROPDOWNLIST);
      }

      new_line();
    }

    label(Far::get_msg(MSG_UPDATE_DLG_LEVEL));
    new_line();
    for (unsigned i = 0; i < ARRAYSIZE(c_levels); i++) {
      if (i)
        spacer(1);
      int ctrl_id = radio_button(Far::get_msg(c_levels[i].name_id), options.level == c_levels[i].value, i == 0 ? DIF_GROUP : 0);
      if (i == 0)
        level_ctrl_id = ctrl_id;
    };
    new_line();

    label(Far::get_msg(MSG_UPDATE_DLG_METHOD));
    new_line();
    for (unsigned i = 0; i < ARRAYSIZE(c_methods); i++) {
      if (i)
        spacer(1);
      int ctrl_id = radio_button(Far::get_msg(c_methods[i].name_id), options.method == c_methods[i].value, i == 0 ? DIF_GROUP : 0);
      if (i == 0)
        method_ctrl_id = ctrl_id;
    };
    new_line();

    solid_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_SOLID), options.solid);
    new_line();
    separator();
    new_line();

    encrypt_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_ENCRYPT), !options.password.empty());
    spacer(2);
    encrypt_header_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_ENCRYPT_HEADER), options.encrypt_header);
    new_line();
    label(Far::get_msg(MSG_UPDATE_DLG_PASSWORD));
    password_ctrl_id = pwd_edit_box(options.password, 20);
    spacer(2);
    label(Far::get_msg(MSG_UPDATE_DLG_PASSWORD2));
    password2_ctrl_id = pwd_edit_box(options.password, 20);
    new_line();
    separator();
    new_line();

    if (new_arc) {
      create_sfx_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_CREATE_SFX), options.create_sfx);
      new_line();
      wstring sfx_module_label = Far::get_msg(MSG_UPDATE_DLG_SFX_MODULE);
      label(sfx_module_label);
      vector<wstring> sfx_module_list;
      const SfxModules& sfx_modules = ArcAPI::sfx();
      for_each(sfx_modules.begin(), sfx_modules.end(), [&] (const SfxModule& sfx_module) {
        sfx_module_list.push_back(sfx_module.path);
      });
      sfx_module_ctrl_id = combo_box(sfx_module_list, options.sfx_module_idx, c_client_xs - get_label_len(sfx_module_label), DIF_DROPDOWNLIST);
      new_line();
      separator();
      new_line();
    }

    move_files_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_MOVE_FILES), options.move_files);
    new_line();
    open_shared_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_OPEN_SHARED), options.open_shared);
    new_line();
    ignore_errors_ctrl_id = check_box(Far::get_msg(MSG_UPDATE_DLG_IGNORE_ERRORS), options.ignore_errors);
    new_line();

    separator();
    new_line();
    ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    return (item != -1) && (item != cancel_ctrl_id);
  }
};

bool update_dialog(bool new_arc, UpdateOptions& options) {
  return UpdateDialog(new_arc, options).show();
}

class SettingsDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 60
  };

  PluginSettings& settings;

  int use_include_masks_ctrl_id;
  int include_masks_ctrl_id;
  int use_exclude_masks_ctrl_id;
  int exclude_masks_ctrl_id;
  int generate_masks_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != cancel_ctrl_id)) {
      settings.use_include_masks = get_check(use_include_masks_ctrl_id);
      settings.include_masks = get_text(include_masks_ctrl_id);
      settings.use_exclude_masks = get_check(use_exclude_masks_ctrl_id);
      settings.exclude_masks = get_text(exclude_masks_ctrl_id);
    }
    else if (msg == DN_INITDIALOG) {
      enable(include_masks_ctrl_id, settings.use_include_masks);
      enable(exclude_masks_ctrl_id, settings.use_exclude_masks);
    }
    else if (msg == DN_BTNCLICK && param1 == use_include_masks_ctrl_id) {
      enable(include_masks_ctrl_id, param2 != 0);
    }
    else if (msg == DN_BTNCLICK && param1 == use_exclude_masks_ctrl_id) {
      enable(exclude_masks_ctrl_id, param2 != 0);
    }
    else if (msg == DN_BTNCLICK && param1 == generate_masks_ctrl_id) {
      generate_masks();
    }
    return default_dialog_proc(msg, param1, param2);
  }

  void generate_masks() {
    const ArcFormats& arc_formats = ArcAPI::formats();
    wstring masks;
    for_each(arc_formats.begin(), arc_formats.end(), [&] (const pair<const ArcType, ArcFormat>& arc_type_format) {
      list<wstring> ext_list = split(arc_type_format.second.extension_list, L' ');
      for_each(ext_list.begin(), ext_list.end(), [&] (const wstring& ext) {
        masks += L"*." + ext + L",";
      });
    });
    if (!masks.empty())
      masks.erase(masks.size() - 1);
    set_text(include_masks_ctrl_id, masks);
  }

public:
  SettingsDialog(PluginSettings& settings): Far::Dialog(Far::get_msg(MSG_PLUGIN_NAME), c_client_xs), settings(settings) {
  }

  bool show() {
    use_include_masks_ctrl_id = check_box(Far::get_msg(MSG_SETTINGS_DLG_USE_INCLUDE_MASKS), settings.use_include_masks);
    new_line();
    include_masks_ctrl_id = edit_box(settings.include_masks, c_client_xs);
    new_line();
    use_exclude_masks_ctrl_id = check_box(Far::get_msg(MSG_SETTINGS_DLG_USE_EXCLUDE_MASKS), settings.use_exclude_masks);
    new_line();
    exclude_masks_ctrl_id = edit_box(settings.exclude_masks, c_client_xs);
    new_line();
    generate_masks_ctrl_id = button(Far::get_msg(MSG_SETTINGS_DLG_GENERATE_MASKS), DIF_BTNNOCLOSE);
    new_line();

    separator();
    new_line();
    ok_ctrl_id = def_button(Far::get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
    cancel_ctrl_id = button(Far::get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = Far::Dialog::show();

    return (item != -1) && (item != cancel_ctrl_id);
  }
};

bool settings_dialog(PluginSettings& settings) {
  return SettingsDialog(settings).show();
}

class AttrDialog: public Far::Dialog {
private:
  const AttrList& attr_list;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if (msg == DN_INITDIALOG) {
      FarDialogItem dlg_item;
      for (unsigned ctrl_id = 0; send_message(DM_GETDLGITEMSHORT, ctrl_id, reinterpret_cast<LONG_PTR>(&dlg_item)); ctrl_id++) {
        if (dlg_item.Type == DI_EDIT) {
          EditorSetPosition esp = { 0 };
          send_message(DM_SETEDITPOSITION, ctrl_id, reinterpret_cast<LONG_PTR>(&esp));
        }
      }
    }
    else if (msg == DN_CTLCOLORDLGITEM) {
      FarDialogItem dlg_item;
      if (send_message(DM_GETDLGITEMSHORT, param1, reinterpret_cast<LONG_PTR>(&dlg_item)) && dlg_item.Type == DI_EDIT) {
        unsigned color = Far::get_colors(COL_DIALOGTEXT);
        return param2 & 0xFF00FF00 | (color << 16) | color;
      }
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  AttrDialog(const AttrList& attr_list): Far::Dialog(Far::get_msg(MSG_ATTR_DLG_TITLE)), attr_list(attr_list) {
  }

  void show() {
    unsigned max_name_len = 0;
    unsigned max_value_len = 0;
    for_each(attr_list.begin(), attr_list.end(), [&] (const Attr& attr) {
      if (attr.name.size() > max_name_len)
        max_name_len = attr.name.size();
      if (attr.value.size() > max_value_len)
        max_value_len = attr.value.size();
    });
    max_value_len += 1;

    unsigned max_width = Far::get_optimal_msg_width();
    if (max_name_len > max_width / 2)
      max_name_len = max_width / 2;
    if (max_name_len + 1 + max_value_len > max_width)
      max_value_len = max_width - max_name_len - 1;

    set_width(max_name_len + 1 + max_value_len);

    for_each(attr_list.begin(), attr_list.end(), [&] (const Attr& attr) {
      label(attr.name, max_name_len);
      spacer(1);
      edit_box(attr.value, max_value_len, DIF_READONLY);
      new_line();
    });

    Far::Dialog::show();
  }
};

void attr_dialog(const AttrList& attr_list) {
  AttrDialog(attr_list).show();
}
