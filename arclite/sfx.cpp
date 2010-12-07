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

void replace_icon(const wstring& pe_path, const wstring& ico_path);
void replace_ver_info(const wstring& pe_path, const VersionInfo& ver_info);

ByteVector generate_install_config(const SfxInstallConfig& config) {
  wstring text;
  text += L";!@Install@!UTF-8!\n";
  if (!config.title.empty())
    text += L"Title=\"" + config.title + L"\"\n";
  if (!config.begin_prompt.empty())
    text += L"BeginPrompt=\"" + config.begin_prompt + L"\"\n";
  if (!config.progress.empty())
    text += L"Progress=\"" + config.progress + L"\"\n";
  if (!config.run_program.empty())
    text += L"RunProgram=\"" + config.run_program + L"\"\n";
  if (!config.directory.empty())
    text += L"Directory=\"" + config.directory + L"\"\n";
  if (!config.execute_file.empty())
    text += L"ExecuteFile=\"" + config.execute_file + L"\"\n";
  if (!config.execute_parameters.empty())
    text += L"ExecuteParameters=\"" + config.execute_parameters + L"\"\n";
  text += L";!@InstallEnd@!\n";
  string utf8_text = unicode_to_ansi(text, CP_UTF8);
  return ByteVector(utf8_text.begin(), utf8_text.end());
}

void create_sfx_module(const wstring& file_path, const SfxOptions& sfx_options) {
  unsigned sfx_id = ArcAPI::sfx().find_by_name(sfx_options.name);
  CHECK(sfx_id < ArcAPI::sfx().size());
  wstring sfx_path = ArcAPI::sfx()[sfx_id].path;
  
  File file;
  file.open(file_path, FILE_WRITE_DATA, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);

  File sfx_file(sfx_path, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
  Buffer<unsigned char> buf(static_cast<size_t>(sfx_file.size()));
  sfx_file.read(buf.data(), static_cast<unsigned>(buf.size()));
  file.write(buf.data(), buf.size());

  file.close();

  if (sfx_options.replace_icon)
    replace_icon(file_path, sfx_options.icon_path);

  if (sfx_options.replace_version)
    replace_ver_info(file_path, sfx_options.ver_info);

  if (sfx_options.append_install_config) {
    file.open(file_path, FILE_WRITE_DATA, FILE_SHARE_READ, OPEN_EXISTING, 0);
    file.set_pos(0, FILE_END);

    ByteVector install_config = generate_install_config(sfx_options.install_config);
    file.write(install_config.data(), install_config.size());
  }
}

void attach_sfx_module(const wstring& file_path, const SfxOptions& sfx_options) {
  AttachSfxModuleProgress progress(file_path);

  {
    OpenOptions options;
    options.arc_path = file_path;
    options.detect = false;
    options.arc_types.push_back(c_7z);
    vector<Archive> archives = Archive::open(options);
    if (archives.empty())
      FAIL_MSG(Far::get_msg(MSG_ERROR_SFX_CONVERT));
    if (!archives.front().is_pure_7z())
      FAIL_MSG(Far::get_msg(MSG_ERROR_SFX_CONVERT));
  }

  FindData file_data = File::get_find_data(file_path);
  progress.set_total(file_data.size());
  wstring dst_path = file_path + c_sfx_ext;
  try {
    create_sfx_module(dst_path, sfx_options);

    File dst_file(dst_path, FILE_WRITE_DATA, FILE_SHARE_READ, OPEN_EXISTING, 0);
    dst_file.set_pos(0, FILE_END);

    File src_file(file_path, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
    Buffer<char> buf(1024 * 1024);
    while (true) {
      unsigned size_read = src_file.read(buf.data(), static_cast<unsigned>(buf.size()));
      if (size_read == 0)
        break;
      CHECK(dst_file.write(buf.data(), size_read) == size_read);
      progress.update_completed(size_read);
    }

    File::set_attr(file_path, file_data.dwFileAttributes);
    dst_file.set_time(file_data.ftCreationTime, file_data.ftLastAccessTime, file_data.ftLastWriteTime);
  }
  catch (...) {
    File::delete_file_nt(dst_path);
    throw;
  }
  File::delete_file_nt(file_path);
}


const GUID c_sfx_options_dialog_guid = { /* 0DCE48E5-B205-44A0-B8BF-96B28E2FD3B3 */
  0x0DCE48E5,
  0xB205,
  0x44A0,
  {0xB8, 0xBF, 0x96, 0xB2, 0x8E, 0x2F, 0xD3, 0xB3}
};

class SfxOptionsDialog: public Far::Dialog {
private:
  enum {
    c_client_xs = 69
  };

  SfxOptions& options;

  int name_ctrl_id;
  int replace_icon_ctrl_id;
  int icon_path_ctrl_id;
  int replace_version_ctrl_id;
  int ver_info_version_ctrl_id;
  int ver_info_comments_ctrl_id;
  int ver_info_company_name_ctrl_id;
  int ver_info_file_description_ctrl_id;
  int ver_info_legal_copyright_ctrl_id;
  int ver_info_product_name_ctrl_id;
  int append_install_config_ctrl_id;
  int install_config_title_ctrl_id;
  int install_config_begin_prompt_ctrl_id;
  int install_config_progress_ctrl_id;
  int install_config_run_program_ctrl_id;
  int install_config_directory_ctrl_id;
  int install_config_execute_file_ctrl_id;
  int install_config_execute_parameters_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  void set_control_state() {
    bool replace_icon = get_check(replace_icon_ctrl_id);
    for (int ctrl_id = replace_icon_ctrl_id + 1; ctrl_id < replace_version_ctrl_id; ctrl_id++)
      enable(ctrl_id, replace_icon);

    bool replace_version = get_check(replace_version_ctrl_id);
    for (int ctrl_id = replace_version_ctrl_id + 1; ctrl_id < append_install_config_ctrl_id; ctrl_id++)
      enable(ctrl_id, replace_version);

    unsigned sfx_id = get_list_pos(name_ctrl_id);
    bool install_config_enabled = sfx_id < ArcAPI::sfx().size() && ArcAPI::sfx()[sfx_id].install_config();
    enable(append_install_config_ctrl_id, install_config_enabled);

    bool append_install_config = get_check(append_install_config_ctrl_id);
    for (int ctrl_id = append_install_config_ctrl_id + 1; ctrl_id <= install_config_execute_parameters_ctrl_id; ctrl_id++)
      enable(ctrl_id, install_config_enabled && append_install_config);
  }

  LONG_PTR dialog_proc(int msg, int param1, LONG_PTR param2) {
    if (msg == DN_CLOSE && param1 >= 0 && param1 != cancel_ctrl_id) {
      const SfxModules& sfx_modules = ArcAPI::sfx();
      unsigned sfx_id = get_list_pos(name_ctrl_id);
      if (sfx_id >= sfx_modules.size()) {
        FAIL_MSG(Far::get_msg(MSG_SFX_OPTIONS_DLG_WRONG_MODULE));
      }
      options.name = extract_file_name(sfx_modules[sfx_id].path);
      options.replace_icon = get_check(replace_icon_ctrl_id);
      if (options.replace_icon)
        options.icon_path = get_text(icon_path_ctrl_id);
      else
        options.icon_path.clear();
      options.replace_version = get_check(replace_version_ctrl_id);
      if (options.replace_version) {
        options.ver_info.version = get_text(ver_info_version_ctrl_id);
        options.ver_info.comments = get_text(ver_info_comments_ctrl_id);
        options.ver_info.company_name = get_text(ver_info_company_name_ctrl_id);
        options.ver_info.file_description = get_text(ver_info_file_description_ctrl_id);
        options.ver_info.legal_copyright = get_text(ver_info_legal_copyright_ctrl_id);
        options.ver_info.product_name = get_text(ver_info_product_name_ctrl_id);
      }
      else {
        options.ver_info.version.clear();
        options.ver_info.comments.clear();
        options.ver_info.company_name.clear();
        options.ver_info.file_description.clear();
        options.ver_info.legal_copyright.clear();
        options.ver_info.product_name.clear();
      }
      bool install_config_enabled = sfx_modules[sfx_id].install_config();
      options.append_install_config = install_config_enabled && get_check(append_install_config_ctrl_id);
      if (options.append_install_config) {
        options.install_config.title = get_text(install_config_title_ctrl_id);
        options.install_config.begin_prompt = get_text(install_config_begin_prompt_ctrl_id);
        TriState value = get_check3(install_config_progress_ctrl_id);
        if (value == triUndef)
          options.install_config.progress.clear();
        else
          options.install_config.progress = value == triTrue ? L"yes" : L"no";
        options.install_config.run_program = get_text(install_config_run_program_ctrl_id);
        options.install_config.directory = get_text(install_config_directory_ctrl_id);
        options.install_config.execute_file = get_text(install_config_execute_file_ctrl_id);
        options.install_config.execute_parameters = get_text(install_config_execute_parameters_ctrl_id);
      }
      else {
        options.install_config.title.clear();
        options.install_config.begin_prompt.clear();
        options.install_config.progress.clear();
        options.install_config.run_program.clear();
        options.install_config.directory.clear();
        options.install_config.execute_file.clear();
        options.install_config.execute_parameters.clear();
      }
    }
    else if (msg == DN_INITDIALOG) {
      set_control_state();
    }
    else if (msg == DN_BTNCLICK) {
      set_control_state();
    }
    else if (msg == DN_EDITCHANGE && param1 == name_ctrl_id) {
      set_control_state();
    }
    return default_dialog_proc(msg, param1, param2);
  }

public:
  SfxOptionsDialog(SfxOptions& options): Far::Dialog(Far::get_msg(MSG_SFX_OPTIONS_DLG_TITLE), &c_sfx_options_dialog_guid, c_client_xs), options(options) {
  }

  bool show() {
    label(Far::get_msg(MSG_SFX_OPTIONS_DLG_NAME));
    vector<wstring> names;
    const SfxModules& sfx_modules = ArcAPI::sfx();
    names.reserve(sfx_modules.size() + 1);
    unsigned name_width = 0;
    for_each(sfx_modules.begin(), sfx_modules.end(), [&] (const SfxModule& sfx_module) {
      wstring name = sfx_module.description();
      names.push_back(name);
      if (name_width < name.size())
        name_width = name.size();
    });
    names.push_back(wstring());
    name_ctrl_id = combo_box(names, sfx_modules.find_by_name(options.name), name_width + 6, DIF_DROPDOWNLIST);
    new_line();

    replace_icon_ctrl_id = check_box(Far::get_msg(MSG_SFX_OPTIONS_DLG_REPLACE_ICON), options.replace_icon);
    new_line();
    spacer(2);
    label(Far::get_msg(MSG_SFX_OPTIONS_DLG_ICON_PATH));
    icon_path_ctrl_id = history_edit_box(options.icon_path, L"arclite.icon_path", AUTO_SIZE, DIF_EDITPATH);
    new_line();

    unsigned label_len = 0;
    vector<wstring> labels;
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_VER_INFO_PRODUCT_NAME));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_VER_INFO_VERSION));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_VER_INFO_COMPANY_NAME));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_VER_INFO_FILE_DESCRIPTION));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_VER_INFO_COMMENTS));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_VER_INFO_LEGAL_COPYRIGHT));
    for (unsigned i = 0; i < labels.size(); i++)
      if (label_len < labels[i].size())
        label_len = labels[i].size();
    label_len += 2;
    vector<wstring>::const_iterator label_text = labels.cbegin();
    replace_version_ctrl_id = check_box(Far::get_msg(MSG_SFX_OPTIONS_DLG_REPLACE_VERSION), options.replace_version);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    ver_info_product_name_ctrl_id = edit_box(options.ver_info.product_name);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    ver_info_version_ctrl_id = edit_box(options.ver_info.version);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    ver_info_company_name_ctrl_id = edit_box(options.ver_info.company_name);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    ver_info_file_description_ctrl_id = edit_box(options.ver_info.file_description);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    ver_info_comments_ctrl_id = edit_box(options.ver_info.comments);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    ver_info_legal_copyright_ctrl_id = edit_box(options.ver_info.legal_copyright);
    new_line();

    label_len = 0;
    labels.clear();
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_INSTALL_CONFIG_TITLE));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_INSTALL_CONFIG_BEGIN_PROMPT));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_INSTALL_CONFIG_PROGRESS));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_INSTALL_CONFIG_RUN_PROGRAM));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_INSTALL_CONFIG_DIRECTORY));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_INSTALL_CONFIG_EXECUTE_FILE));
    labels.push_back(Far::get_msg(MSG_SFX_OPTIONS_DLG_INSTALL_CONFIG_EXECUTE_PARAMETERS));
    for (unsigned i = 0; i < labels.size(); i++)
      if (label_len < labels[i].size())
        label_len = labels[i].size();
    label_len += 2;
    label_text = labels.cbegin();
    append_install_config_ctrl_id = check_box(Far::get_msg(MSG_SFX_OPTIONS_DLG_APPEND_INSTALL_CONFIG), options.append_install_config);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    install_config_title_ctrl_id = edit_box(options.install_config.title);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    install_config_begin_prompt_ctrl_id = edit_box(options.install_config.begin_prompt);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    TriState value;
    if (options.install_config.progress == L"yes")
      value = triTrue;
    else if (options.install_config.progress == L"no")
      value = triFalse;
    else
      value = triUndef;
    install_config_progress_ctrl_id = check_box3(wstring(), value);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    install_config_run_program_ctrl_id = edit_box(options.install_config.run_program);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    install_config_directory_ctrl_id = edit_box(options.install_config.directory);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    install_config_execute_file_ctrl_id = edit_box(options.install_config.execute_file);
    new_line();
    spacer(2);
    label(*label_text++);
    pad(label_len);
    install_config_execute_parameters_ctrl_id = edit_box(options.install_config.execute_parameters);
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

bool sfx_options_dialog(SfxOptions& sfx_options) {
  return SfxOptionsDialog(sfx_options).show();
}
