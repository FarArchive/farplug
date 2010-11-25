#include "msg.h"
#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "common.hpp"
#include "archive.hpp"
#include "ui.hpp"

class AttachSfxModuleProgress: public ProgressMonitor {
private:
  wstring file_path;
  unsigned __int64 completed;
  unsigned __int64 total;

  virtual void do_update_ui() {
    const unsigned c_width = 60;

    percent_done = calc_percent(completed, total);

    wostringstream st;
    st << fit_str(file_path, c_width) << L'\n';
    st << Far::get_progress_bar_str(c_width, percent_done, 100) << L'\n';
    progress_text = st.str();
  }

public:
  AttachSfxModuleProgress(const wstring& file_path): ProgressMonitor(Far::get_msg(MSG_PROGRESS_SFX_CONVERT)), file_path(file_path), completed(0), total(0) {
  }

  void set_total(unsigned __int64 size) {
    total = size;
  }
  void update_completed(unsigned __int64 size) {
    completed += size;
    update_ui();
  }
};

void append_file(const wstring& src_path, File& dst_file, AttachSfxModuleProgress& progress) {
  File source(src_path, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING, 0);
  Buffer<char> buf(1024 * 1024);
  while (true) {
    unsigned size_read = source.read(buf.data(), static_cast<unsigned>(buf.size()));
    if (size_read == 0)
      break;
    CHECK(dst_file.write(buf.data(), size_read) == size_read);
    progress.update_completed(size_read);
  }
}

void attach_sfx_module(const wstring& file_path, const wstring& sfx_module) {
  AttachSfxModuleProgress progress(file_path);

  {
    OpenOptions options;
    options.arc_path = file_path;
    options.detect = false;
    ArcFormats arc_formats;
    arc_formats[c_7z] = ArcAPI::formats().at(c_7z);
    vector<Archive> archives = Archive::open(options, arc_formats);
    if (archives.empty())
      FAIL_MSG(Far::get_msg(MSG_ERROR_SFX_CONVERT));
    if (!archives.front().is_pure_7z())
      FAIL_MSG(Far::get_msg(MSG_ERROR_SFX_CONVERT));
  }

  FindData file_data = File::get_find_data(file_path);
  progress.set_total(file_data.size());

  wstring dst_path = file_path + c_sfx_ext;
  try {
    File dst_file(dst_path, FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, CREATE_NEW, FILE_ATTRIBUTE_NORMAL);
    append_file(sfx_module, dst_file, progress);
    append_file(file_path, dst_file, progress);
    File::set_attr(file_path, file_data.dwFileAttributes);
    dst_file.set_time(file_data.ftCreationTime, file_data.ftLastAccessTime, file_data.ftLastWriteTime);
  }
  catch (...) {
    File::delete_file_nt(dst_path);
    throw;
  }
  File::delete_file_nt(file_path);
}


const GUID c_sfx_convert_dialog_guid = { /* 0DCE48E5-B205-44A0-B8BF-96B28E2FD3B3 */
  0x0DCE48E5,
  0xB205,
  0x44A0,
  {0xB8, 0xBF, 0x96, 0xB2, 0x8E, 0x2F, 0xD3, 0xB3}
};

class SfxConvertDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 69
  };

  wstring& sfx_module;

  int sfx_module_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if (msg == DN_CLOSE && param1 >= 0 && param1 != cancel_ctrl_id) {
      sfx_module = get_text(sfx_module_ctrl_id);
      if (sfx_module.empty()) {
        FAIL_MSG(Far::get_msg(MSG_UPDATE_DLG_WRONG_SFX_MODULE));
      }
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  SfxConvertDialog(wstring& sfx_module): Far::Dialog(Far::get_msg(MSG_SFX_CONVERT_DLG_TITLE), &c_sfx_convert_dialog_guid, c_client_xs), sfx_module(sfx_module) {
  }

  bool show() {
    label(Far::get_msg(MSG_SFX_CONVERT_DLG_MODULE));
    new_line();
    vector<wstring> sfx_module_list;
    const SfxModules& sfx_modules = ArcAPI::sfx();
    sfx_module_list.reserve(sfx_modules.size() + 1);
    for_each(sfx_modules.begin(), sfx_modules.end(), [&] (const wstring& sfx_module) {
      sfx_module_list.push_back(sfx_module);
    });
    sfx_module_list.push_back(wstring());
    sfx_module_ctrl_id = combo_box(sfx_module_list, sfx_modules.find(sfx_module), c_client_xs, DIF_DROPDOWNLIST | DIF_EDITPATH);
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

bool sfx_convert_dialog(wstring& sfx_module) {
  return SfxConvertDialog(sfx_module).show();
}
