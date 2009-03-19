#include <windows.h>

#include "rapi2.h"
#include "replfilt.h"

#include "plugin.hpp"

#include "col/AnsiString.h"
#include "col/UnicodeString.h"
#include "col/PlainArray.h"
#include "col/ObjectArray.h"
using namespace col;

#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "msg.h"

#include "util.h"
#include "options.h"
#include "dlgapi.h"
#include "file_filters.h"
#include "file_info.h"
#include "ui.h"
#include "filepath.h"
#include "plugin.h"
#include "rapi_proc.h"

extern "C" HINSTANCE g_h_module;
HINSTANCE g_h_module;
struct PluginStartupInfo g_far;
struct FarStandardFunctions g_fsf;
Array<unsigned char> g_colors;
PluginOptions g_plugin_options;
ModuleVersion g_version;
unsigned __int64 g_time_freq; // system timer resolution
Array<PluginInstance*> g_plugin_objects; // list of all open plugin instances
FarStr g_plugin_format;

UnicodeString DeviceInfo::strid() {
  const unsigned size = 64;
  UnicodeString res;
  VERIFY(StringFromGUID2(id, res.buf(size), size) != 0);
  res.set_size();
  return res;
}

void error_dlg(Error& e, const UnicodeString& message) {
  UnicodeString msg;
  msg.add(far_get_msg(MSG_PLUGIN_NAME)).add('\n');
  if (message.size() != 0) msg.add(word_wrap(message, get_msg_width())).add('\n');
  UnicodeString err_msg = word_wrap(e.message(), get_msg_width());
  if (err_msg.size() != 0) msg.add(err_msg).add('\n');
  msg.add_fmt(L"%S:%u r.%u"PLUGIN_TYPE, &extract_file_name(oem_to_unicode(e.file)), e.line, g_version.revision);
  far_message(msg, 0, FMSG_WARNING | FMSG_MB_OK);
}

void error_dlg(CustomError& e) {
  UnicodeString msg;
  msg.add(far_get_msg(MSG_PLUGIN_NAME)).add('\n');
  if (e.object().size() != 0) msg.add(fit_str(e.object(), get_msg_width())).add('\n');
  if (e.message().size() != 0) msg.add(word_wrap(e.message(), get_msg_width())).add('\n');
  msg.add_fmt(L"%S:%u r.%u"PLUGIN_TYPE, &extract_file_name(oem_to_unicode(e.file)), e.line, g_version.revision);
  far_message(msg, 0, FMSG_WARNING | FMSG_MB_OK);
}

void error_dlg(const std::exception& e) {
  UnicodeString msg;
  msg.add(far_get_msg(MSG_PLUGIN_NAME)).add('\n');
  UnicodeString err_msg = word_wrap(oem_to_unicode(e.what()), get_msg_width());
  if (err_msg.size() != 0) msg.add(err_msg).add('\n');
  msg.add_fmt(L"r.%u"PLUGIN_TYPE, g_version.revision);
  far_message(msg, 0, FMSG_WARNING | FMSG_MB_OK);
}

void msg_dlg(const UnicodeString& message) {
  UnicodeString msg;
  msg.add(far_get_msg(MSG_PLUGIN_NAME)).add('\n');
  if (message.size() != 0) msg.add(word_wrap(message, get_msg_width())).add('\n');
  far_message(msg, 0, FMSG_MB_OK);
}

void fatal_error_dlg(bool dump_saved) {
  const FarCh* msg[3];
  msg[0] = far_msg_ptr(MSG_PLUGIN_NAME);
  msg[1] = far_msg_ptr(MSG_ERR_FATAL);
  msg[2] = far_msg_ptr(dump_saved ? MSG_ERR_FATAL_DUMP : MSG_ERR_FATAL_NO_DUMP);
  far_message(msg, sizeof(msg) / sizeof(msg[0]), 0, FMSG_MB_OK);
}

struct SetFileApis {
  SetFileApis() {
    SetFileApisToANSI();
  }
  ~SetFileApis() {
    SetFileApisToOEM();
  }
};

#ifdef FARAPI17
int WINAPI GetMinFarVersion(void) {
  return MAKEFARVERSION(1, 70, 2087);
}
#endif // FARAPI17

#ifdef FARAPI18
int WINAPI GetMinFarVersion(void) {
  return FARMANAGERVERSION;
}
int WINAPI GetMinFarVersionW(void) {
  return FARMANAGERVERSION;
}
#endif // FARAPI18

void WINAPI FAR_EXPORT(SetStartupInfo)(const struct PluginStartupInfo* info) {
  SetFileApis set_file_apis;
  g_far = *info;
  g_fsf = *info->FSF;
  g_far.FSF = &g_fsf;
  // measure system timer frequency
  QueryPerformanceFrequency((PLARGE_INTEGER) &g_time_freq);
  // load Far color table
  far_load_colors();
  // load registry options
  load_plugin_options(g_plugin_options);
  g_rapi2 = g_plugin_options.access_method == amRapi2;
  g_plugin_format = UNICODE_TO_FARSTR(g_plugin_options.prefix);
  // load version info
  g_version = get_module_version(g_h_module);
}

const FarCh* disk_menu[1];
const FarCh* plugin_menu[1];
const FarCh* config_menu[1];
void WINAPI FAR_EXPORT(GetPluginInfo)(struct PluginInfo *Info) {
  SetFileApis set_file_apis;
  Info->StructSize = sizeof(struct PluginInfo);

  ObjectArray<DeviceInfo> dev_list;
  try {
    init_if_needed();
    get_device_list(dev_list);
  }
  catch (...) {
    dev_list.clear();
  }
  if (dev_list.size() == 0) {
    if (g_plugin_options.last_dev_type == dtPDA) disk_menu[0] = far_msg_ptr(MSG_DISK_MENU_PDA);
    else if (g_plugin_options.last_dev_type == dtSmartPhone) disk_menu[0] = far_msg_ptr(MSG_DISK_MENU_SMARTPHONE);
    else disk_menu[0] = far_msg_ptr(MSG_DISK_MENU_PDA);
  }
  else {
    if (dev_list[0].platform == L"PocketPC") {
      disk_menu[0] = far_msg_ptr(MSG_DISK_MENU_PDA);
      g_plugin_options.last_dev_type = dtPDA;
    }
    else if (dev_list[0].platform == L"SmartPhone") {
      disk_menu[0] = far_msg_ptr(MSG_DISK_MENU_SMARTPHONE);
      g_plugin_options.last_dev_type = dtSmartPhone;
    }
    else {
      disk_menu[0] = far_msg_ptr(MSG_DISK_MENU_PDA);
      g_plugin_options.last_dev_type = dtPDA;
    }
    save_plugin_options(g_plugin_options);
  }

  if (g_plugin_options.add_to_disk_menu) {
    Info->DiskMenuStrings = disk_menu;
    if (g_plugin_options.disk_menu_number == -1) Info->DiskMenuNumbers = NULL;
    else Info->DiskMenuNumbers = &g_plugin_options.disk_menu_number;
    Info->DiskMenuStringsNumber = sizeof(disk_menu) / sizeof(disk_menu[0]);
  }

  if (g_plugin_options.add_to_plugin_menu) {
    plugin_menu[0] = far_msg_ptr(MSG_PLUGIN_NAME);
    Info->PluginMenuStrings = plugin_menu;
    Info->PluginMenuStringsNumber = sizeof(plugin_menu) / sizeof(plugin_menu[0]);
  }

  config_menu[0] = far_msg_ptr(MSG_PLUGIN_NAME);
  Info->PluginConfigStrings = config_menu;
  Info->PluginConfigStringsNumber = sizeof(config_menu) / sizeof(config_menu[0]);

  Info->CommandPrefix = g_plugin_format.data();
}

#define HANDLE_ERROR(err_ret, break_ret) \
  catch (Break&) { \
    return break_ret; \
  } \
  catch (CustomError& e) { \
    if (show_error) error_dlg(e); \
    return err_ret; \
  } \
  catch (MsgError& e) { \
    if (show_error) msg_dlg(e.message()); \
    return err_ret; \
  } \
  catch (ComError& e) { \
    if (e.error() == HRESULT_FROM_WIN32(ERROR_NOT_READY)) { \
      far_control_ptr(plugin, FCTL_CLOSEPLUGIN, NULL); \
      if (show_error) msg_dlg(far_get_msg(MSG_ERR_DEVICE_DISCONNECTED)); \
    } \
    else { \
      LOG_ERR(e); \
      if (show_error) error_dlg(e, fit_str(plugin->last_object, get_msg_width())); \
    } \
    return err_ret; \
  } \
  catch (Error& e) { \
    LOG_ERR(e); \
    if (show_error) error_dlg(e, fit_str(plugin->last_object, get_msg_width())); \
    return err_ret; \
  } \
  catch (std::exception& e) { \
    if (show_error) error_dlg(e); \
    return err_ret; \
  } \
  catch (...) { \
    far_message(L"\nFailure!", 0, FMSG_WARNING | FMSG_MB_OK); \
    return err_ret; \
  }

int WINAPI FAR_EXPORT(SetDirectory)(HANDLE hPlugin, const FarCh *Dir, int OpMode); // forward

HANDLE WINAPI FAR_EXPORT(OpenPlugin)(int OpenFrom, INT_PTR Item) {
  SetFileApis set_file_apis;
  try {
    if (OpenFrom == OPEN_COMMANDLINE) {
      // test if plugin is already open on active panel
      for (unsigned i = 0; i < g_plugin_objects.size(); i++) {
        PanelInfo pi;
        if (far_control_ptr(g_plugin_objects[i], FCTL_GETPANELSHORTINFO, &pi)) {
          if (pi.Focus) {
            // change current directory of active plugin
            FAR_EXPORT(SetDirectory)(g_plugin_objects[i], (const FarCh*) Item, 0);
            far_control_int(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, 0);
            PanelRedrawInfo pri;
            pri.CurrentItem = 0;
            pri.TopPanelItem = 0;
            far_control_ptr(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, &pri);
            // do not create new plugin instance
            return INVALID_HANDLE_VALUE;
          }
        }
      }
    }
    init_if_needed();
    ObjectArray<DeviceInfo> dev_list;
    get_device_list(dev_list);
    if (dev_list.size() == 0) FAIL(MsgError(far_get_msg(MSG_ERR_NO_DEVICE)));
    int mi = 0;
    if (dev_list.size() > 1) {
      ObjectArray<UnicodeString> menu_str;
      for (unsigned i = 0; i < dev_list.size(); i++) {
        menu_str += UnicodeString::format(L"%S %S (%S)", &dev_list[i].name, &dev_list[i].platform, &dev_list[i].con_type);
      }
      mi = far_menu(far_get_msg(MSG_DEVICE_LIST_TITLE), menu_str);
      if (mi == -1) BREAK;
    }
    PluginInstance* plugin = new PluginInstance();
    try {
      plugin->device_info = dev_list[mi];
      create_session(plugin->device_info.id, plugin);
      if (OpenFrom == OPEN_COMMANDLINE) {
        // directory is specified on command line
        plugin->current_dir = FARSTR_TO_UNICODE((const FarCh*) Item);
        if (!dir_exists(plugin->current_dir, plugin->session)) plugin->current_dir = L"\\";
      }
      else if (g_plugin_options.save_last_dir) {
        // restore last directory
        plugin->current_dir = get_str_option((L"last_dir_" + plugin->device_info.strid()).data(), L"\\");
        if (!dir_exists(plugin->current_dir, plugin->session)) plugin->current_dir = L"\\";
      }
      else {
        plugin->current_dir = L"\\";
      }
#ifdef FARAPI17
      plugin->current_dir_oem = unicode_to_oem(encode_fn(plugin->current_dir));
#endif // FARAPI17
      NOFAIL(refresh_system_info(plugin));
      g_plugin_objects += plugin;
    }
    catch (...) {
      delete plugin;
      throw;
    }
    return plugin;
  }
  catch (Break&) {
  }
  catch (MsgError& e) {
    msg_dlg(e.message());
  }
  catch (Error& e) {
    LOG_ERR(e);
    error_dlg(e, far_get_msg(MSG_ERR_PLUGIN_INIT));
  }
  catch (...) {
    far_message(L"\nFailure!", 0, FMSG_WARNING | FMSG_MB_OK);
  }
  return INVALID_HANDLE_VALUE;
}

void WINAPI FAR_EXPORT(GetOpenPluginInfo)(HANDLE hPlugin, struct OpenPluginInfo *Info) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;

  Info->StructSize = sizeof(struct OpenPluginInfo);

  Info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS;
#ifdef FARAPI17
  Info->CurDir = plugin->current_dir_oem.data();
#endif // FARAPI17
#ifdef FARAPI18
  Info->CurDir = plugin->current_dir.data();
#endif // FARAPI18
  Info->Format = g_plugin_format.data();

  if (g_plugin_options.last_dev_type == dtPDA) plugin->panel_title = far_msg_ptr(MSG_DISK_MENU_PDA);
  else if (g_plugin_options.last_dev_type == dtSmartPhone) plugin->panel_title = far_msg_ptr(MSG_DISK_MENU_SMARTPHONE);
  else plugin->panel_title = far_msg_ptr(MSG_DISK_MENU_PDA);
#ifdef FARAPI17
  plugin->panel_title.add(":").add(plugin->current_dir_oem);
#endif // FARAPI17
#ifdef FARAPI18
  plugin->panel_title.add(L":").add(plugin->current_dir);
#endif // FARAPI18

  if (g_plugin_options.show_free_space) plugin->panel_title.add(UNICODE_TO_FARSTR(L":" + format_data_size(plugin->free_space, get_size_suffixes())));
  Info->PanelTitle = plugin->panel_title.data();
  Info->InfoLines = plugin->sys_info.data();
  Info->InfoLinesNumber = plugin->sys_info.size();
}

void WINAPI FAR_EXPORT(ClosePlugin)(HANDLE hPlugin) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  end_session(plugin);
  // save last directory
  if (g_plugin_options.save_last_dir) {
    set_str_option((L"last_dir_" + plugin->device_info.strid()).data(), plugin->current_dir);
  }
  g_plugin_objects.remove(g_plugin_objects.search(plugin));
  delete plugin;
}

int WINAPI FAR_EXPORT(GetFindData)(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  bool show_error = (OpMode & (OPM_SILENT | OPM_FIND | OPM_QUICKVIEW)) == 0;
  try {
    UiLink ui((OpMode & OPM_SILENT) != 0);
    PluginItemList file_list;
    plugin->last_object = plugin->current_dir;
    FileListOptions options;
    options.hide_rom_files = g_plugin_options.hide_rom_files;
    gen_file_list(plugin->current_dir, plugin->session, file_list, options, ui);
    plugin->file_lists += file_list;
    *pPanelItem = (PluginPanelItem*) plugin->file_lists.last().data();
    *pItemsNumber = plugin->file_lists.last().size();
    if (!(OpMode & OPM_FIND)) NOFAIL(refresh_system_info(plugin));
    return TRUE;
  }
  HANDLE_ERROR(FALSE, TRUE);
}

void WINAPI FAR_EXPORT(FreeFindData)(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  for (unsigned i = 0; i < plugin->file_lists.size(); i++) {
    if (plugin->file_lists[i].data() == PanelItem) {
      plugin->file_lists.remove(i);
      return;
    }
  }
  assert(false);
}

int WINAPI FAR_EXPORT(SetDirectory)(HANDLE hPlugin, const FarCh *Dir, int OpMode) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  bool show_error = (OpMode & (OPM_SILENT | OPM_FIND | OPM_QUICKVIEW)) == 0;
  try {
    FilePath fp_dir(FAR_DECODE_PATH(Dir));
    FilePath current_dir(plugin->current_dir);
    bool parent_dir = FAR_STRCMP(Dir, FAR_T("..")) == 0;
    if (current_dir.is_root_path() && (parent_dir || (FAR_STRCMP(Dir, FAR_T("\\")) == 0))) {
      if (g_plugin_options.exit_on_dot_dot) far_control_ptr(plugin, FCTL_CLOSEPLUGIN, NULL);
      return TRUE;
    }
    else if (parent_dir) {
      current_dir = current_dir.get_partial_path(current_dir.size() - 1);
    }
    else {
      current_dir.combine(fp_dir);
    }
    UnicodeString cd = current_dir.get_full_path();
    if (!current_dir.is_root_path()) {
      plugin->last_object = cd;
      if (!dir_exists(cd, plugin->session)) FAIL(SystemError(ERROR_PATH_NOT_FOUND));
    }
    plugin->current_dir = cd;
#ifdef FARAPI17
    plugin->current_dir_oem = unicode_to_oem(encode_fn(plugin->current_dir));
#endif // FARAPI17
    return TRUE;
  }
  HANDLE_ERROR(FALSE, TRUE);
}

int get_files(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, UnicodeString& DestPath, int OpMode) {
  if ((ItemsNumber == 0) || (FAR_STRCMP(FAR_FILE_NAME(PanelItem[0].FindData), FAR_T("..")) == 0)) return 1;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  bool show_dialog = (OpMode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
  bool show_error = (OpMode & (OPM_FIND | OPM_QUICKVIEW)) == 0;
  try {
    CopyFilesOptions options;
    options.show_dialog = show_dialog;
    options.show_error = show_error;
    options.dst_dir = DestPath;
    options.move_files = Move != 0;
    INT_PTR far_system_settings = g_far.AdvControl(g_far.ModuleNumber, ACTL_GETSYSTEMSETTINGS, NULL);
    options.copy_shared = (far_system_settings & FSS_COPYFILESOPENEDFORWRITING) != 0;
    options.use_file_filters = false;
    if (show_dialog) {
      options.ignore_errors = g_plugin_options.ignore_errors;
      options.overwrite = g_plugin_options.overwrite;
      options.show_stats = g_plugin_options.show_stats;
      if (!show_copy_files_dlg(options, false)) BREAK;
      if (g_plugin_options.save_def_values) {
        g_plugin_options.ignore_errors = options.ignore_errors;
        g_plugin_options.overwrite = options.overwrite;
        g_plugin_options.show_stats = options.show_stats;
        save_def_option_values(g_plugin_options);
      }
      DestPath = options.dst_dir;
    }
    else {
      options.ignore_errors = false;
      options.overwrite = ooOverwrite;
      options.show_stats = ssoNever;
    }
    CopyFilesStats stats;
    Log log;
    {
      UiLink ui((OpMode & OPM_SILENT) != 0);
      if (ui.update_needed()) {
        draw_progress_msg(far_get_msg(MSG_PROGRESS_PREPARE));
      }

      // source directory
      UnicodeString src_dir_path = plugin->current_dir;

      // distination directory and file name (if renaming)
      UnicodeString dst_dir_path, dst_new_name;
      FilePath dst_fp(options.dst_dir);
      bool dst_is_remote = !dst_fp.is_absolute || (dst_fp.root.size() == 0);
      if (dst_is_remote) {
        // ensure that file name case is correct in source and destinations paths
        // it will be used later in comparison
        FilePath src_dir_fp(src_dir_path);
        find_real_file_path(src_dir_fp, plugin->session);
        src_dir_path = src_dir_fp.get_full_path();
        dst_fp = FilePath(plugin->current_dir).combine(dst_fp);
        find_real_file_path(dst_fp, plugin->session);
      }
      if ((dst_is_remote && dir_exists(dst_fp.get_full_path(), plugin->session)) || (!dst_is_remote && dir_exists(dst_fp.get_full_path()))) {
        dst_dir_path = dst_fp.get_full_path();
      }
      else {
        if (ItemsNumber != 1) {
          dst_dir_path = dst_fp.get_full_path();
        }
        else {
          dst_dir_path = dst_fp.get_dir_path();
          dst_new_name = dst_fp.get_file_name();
        }
      }

      UnicodeString src_file_name, dst_file_name; // store source / destination file names
      UnicodeString src_path, dst_path; // store source / destination file paths

      // list of selected files
      FileList panel_file_list;
      panel_items_to_file_list(PanelItem, ItemsNumber, panel_file_list);

      // verify that no file is copied into self
      if (dst_is_remote) {
        for (int i = 0; i < ItemsNumber; i++) {
          src_file_name = panel_file_list[i].file_name;
          COMPOSE_PATH2(src_path, src_dir_path, src_file_name);
          if (dst_new_name.size() != 0) dst_file_name = dst_new_name; else dst_file_name = src_file_name;
          COMPOSE_PATH2(dst_path, dst_dir_path, dst_file_name);
          if (dst_path == src_path) FAIL(CustomError(far_get_msg(MSG_ERR_SELF_COPY), src_path));
        }
      }

      // make sure destination path exists
      if (dst_is_remote) prepare_target_path(dst_dir_path, plugin->session, plugin);
      else prepare_target_path(dst_dir_path, plugin);

      Array<unsigned> finished_idx; // indices of processed files

      // try to move files remotely
      // mark files that were processed successfully
      if (options.move_files && dst_is_remote) {
        // prepare progress data
        CopyFilesProgress progress;
        QueryPerformanceCounter((PLARGE_INTEGER) &progress.start_time);
        ui.force_update();
        // iterate through selected files
        for (int i = 0; i < ItemsNumber; i++) {
          src_file_name = panel_file_list[i].file_name; // source file name
          COMPOSE_PATH2(src_path, src_dir_path, src_file_name); // source file path
          if (dst_new_name.size() != 0) dst_file_name = dst_new_name; else dst_file_name = src_file_name; // destination file name
          COMPOSE_PATH2(dst_path, dst_dir_path, dst_file_name); // destination file path
          // update progress bar if needed
          if (ui.update_needed()) {
            progress.src_path = src_path;
            progress.dst_path = dst_path;
            draw_move_remote_files_progress(progress, stats);
          }
          // try to move file remotely
          if (move_remote_file(src_path, dst_path, plugin->session)) {
            // update stats
            if (panel_file_list[i].is_dir()) stats.dirs++;
            else stats.files++;
            // add finished file to list
            finished_idx += i;
          }
        }
      }

      // scan source directories and prepare lists of files to process
      ObjectArray<FileList> file_lists;
      ui.force_update();
      CreateListStats list_stats;
      CreateListOptions list_options;
      list_options.ignore_errors = options.ignore_errors;
      list_options.show_error = options.show_error;
      try {
        for (int i = 0; i < ItemsNumber; i++) {
          if (finished_idx.bsearch(i) == -1) {
            file_lists += create_file_list(src_dir_path, panel_file_list[i].file_name, list_stats, list_options, ui, log, plugin->session, plugin);
          }
          else file_lists += FileList(); // skip already moved objects
        }
      }
      finally (stats.errors = list_stats.errors);

      // show file filters dialog if needed
      ObjectArray<FilterInterface> filters;
      if (options.use_file_filters && !dst_is_remote && show_dialog) {
        load_file_filters();
        if (export_filter_list.size() != 0) {
          Array<FilterSelection> selection;
          if (!show_filters_dlg(export_filter_list, selection)) BREAK;
          for (unsigned i = 0; i < selection.size(); i++) {
            filters += FilterInterface(export_filter_list[selection[i].src_idx].src_ext, export_filter_list[selection[i].src_idx][selection[i].dst_idx].dst_ext, export_filter_list[selection[i].src_idx][selection[i].dst_idx].guid);
          }
        }
      }

      // perform copy
      CopyFilesProgress progress;
      progress.total_size = list_stats.size;
      progress.processed_total_size = progress.copied_total_size = 0;
      QueryPerformanceCounter((PLARGE_INTEGER) &progress.start_time);
      ui.force_update();
      AutoBuffer buffer(g_plugin_options.copy_buf_size);
      for (int i = 0; i < ItemsNumber; i++) {
        if (finished_idx.bsearch(i) == -1) {
          copy_files(true, src_dir_path, file_lists[i], dst_is_remote, dst_dir_path, dst_new_name, stats, progress, options, ui, buffer, log, filters, plugin->session, plugin);
        }
        PanelItem[i].Flags &= ~PPIF_SELECTED;
      }

      // delete source files if moving (only if no errors or skipped files to prevent data loss)
      if (options.move_files && (stats.errors == 0) && (stats.skipped == 0)) {
        DeleteFilesStats del_stats;
        DeleteFilesOptions del_options;
        del_options.ignore_errors = options.ignore_errors;
        del_options.show_stats = options.show_stats;
        del_options.show_error = options.show_error;
        del_options.show_dialog = options.show_dialog;
        DeleteFilesProgress del_progress;
        del_progress.objects = 0;
        del_progress.total_objects = list_stats.files + list_stats.dirs;
        QueryPerformanceCounter((PLARGE_INTEGER) &del_progress.start_time);
        ui.force_update();
        try {
          for (int i = 0; i < ItemsNumber; i++) {
            delete_files(true, src_dir_path, file_lists[i], del_stats, del_progress, del_options, ui, log, plugin->session, plugin);
          }
        }
        finally (stats.errors += del_stats.errors);
      }

      // set cursor to new file name after rename
      if (dst_is_remote && options.move_files && (src_dir_path == dst_dir_path)) {
        assert(dst_new_name.size() != 0);
        far_control_int(plugin, FCTL_UPDATEPANEL, 1);
        PanelInfo panel_info;
        far_control_ptr(plugin, FCTL_GETPANELINFO, &panel_info);
        PanelRedrawInfo redraw_info;
        redraw_info.TopPanelItem = panel_info.TopPanelItem;
        redraw_info.CurrentItem = panel_info.CurrentItem;
        for (int i = 0; i < panel_info.ItemsNumber; i++) {
          PluginPanelItem* ppi = far_get_panel_item(plugin, i, panel_info);
          UnicodeString file_name = FAR_DECODE_PATH(FAR_FILE_NAME(ppi->FindData));
          if (file_name == dst_new_name) {
            redraw_info.CurrentItem = i;
            break;
          }
        }
        far_control_ptr(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, &redraw_info);
      }
    }

    if (show_dialog && ((options.show_stats == ssoAlways) || ((options.show_stats == ssoIfError) && (stats.errors != 0)))) show_copy_files_results_dlg(stats, log);
    return 1;
  }
  HANDLE_ERROR(0, -1);
}

int WINAPI FAR_EXPORT(GetFiles)(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, FarBuf DestPath, int OpMode) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  UnicodeString dest_path = FAR_GET_BUF(DestPath);
  int res = get_files(hPlugin, PanelItem, ItemsNumber, Move, dest_path, OpMode);
  FAR_SET_BUF(DestPath, dest_path, plugin->dest_path_buf);
  return res;
}

int WINAPI FAR_EXPORT(PutFiles)(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode) {
  SetFileApis set_file_apis;
  if ((ItemsNumber == 0) || (FAR_STRCMP(FAR_FILE_NAME(PanelItem[0].FindData), FAR_T("..")) == 0)) return 1;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  bool show_dialog = (OpMode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
  bool show_error = (OpMode & (OPM_FIND | OPM_QUICKVIEW)) == 0;
  try {
    CopyFilesOptions options;
    options.show_dialog = show_dialog;
    options.show_error = show_error;
    options.dst_dir = plugin->current_dir;
    options.move_files = Move != 0;
    INT_PTR far_system_settings = g_far.AdvControl(g_far.ModuleNumber, ACTL_GETSYSTEMSETTINGS, NULL);
    options.copy_shared = (far_system_settings & FSS_COPYFILESOPENEDFORWRITING) != 0;
    if (show_dialog) {
      options.ignore_errors = g_plugin_options.ignore_errors;
      options.overwrite = g_plugin_options.overwrite;
      options.show_stats = g_plugin_options.show_stats;
      options.use_file_filters = g_plugin_options.use_file_filters;
      if (!g_plugin_options.hide_copy_dlg) {
        if (!show_copy_files_dlg(options, true)) BREAK;
        if (g_plugin_options.save_def_values) {
          g_plugin_options.ignore_errors = options.ignore_errors;
          g_plugin_options.overwrite = options.overwrite;
          g_plugin_options.show_stats = options.show_stats;
          g_plugin_options.use_file_filters = options.use_file_filters;
          save_def_option_values(g_plugin_options);
        }
      }
    }
    else {
      options.ignore_errors = false;
      options.overwrite = ooOverwrite;
      options.show_stats = ssoNever;
      options.use_file_filters = false;
    }
    CopyFilesStats stats;
    Log log;
    {
      UiLink ui((OpMode & OPM_SILENT) != 0);
      if (ui.update_needed()) {
        draw_progress_msg(far_get_msg(MSG_PROGRESS_PREPARE));
      }

      // distination directory and file name (if renaming)
      UnicodeString dst_dir_path, dst_new_name;
      FilePath dst_fp(plugin->current_dir);
      if (UNICODE_TO_FARSTR(plugin->current_dir) != UNICODE_TO_FARSTR(options.dst_dir)) {
        dst_fp.combine(options.dst_dir);
      }
      if (dir_exists(dst_fp.get_full_path(), plugin->session)) {
        dst_dir_path = dst_fp.get_full_path();
      }
      else {
        if (ItemsNumber != 1) {
          dst_dir_path = dst_fp.get_full_path();
        }
        else {
          dst_dir_path = dst_fp.get_dir_path();
          dst_new_name = dst_fp.get_file_name();
        }
        // make sure destination path exists
        prepare_target_path(dst_dir_path, plugin->session, plugin);
      }

      // list of selected files
      PanelFileList panel_file_list;
      file_panel_items_to_file_list(PanelItem, ItemsNumber, panel_file_list, ui, plugin);

      // scan source directories and prepare lists of files to process
      ObjectArray<FileList> file_lists;
      ui.force_update();
      CreateListStats list_stats;
      CreateListOptions list_options;
      list_options.ignore_errors = options.ignore_errors;
      list_options.show_error = options.show_error;
      try {
        for (int i = 0; i < ItemsNumber; i++) {
          file_lists += create_file_list(panel_file_list[i].file_dir, panel_file_list[i].file_name, list_stats, list_options, ui, log, plugin);
        }
      }
      finally (stats.errors = list_stats.errors);

      // show file filters dialog if needed
      ObjectArray<FilterInterface> filters;
      if (options.use_file_filters && !g_plugin_options.hide_copy_dlg && show_dialog) {
        load_file_filters();
        if (import_filter_list.size() != 0) {
          Array<FilterSelection> selection;
          if (!show_filters_dlg(import_filter_list, selection)) BREAK;
          for (unsigned i = 0; i < selection.size(); i++) {
            filters += FilterInterface(import_filter_list[selection[i].src_idx].src_ext, import_filter_list[selection[i].src_idx][selection[i].dst_idx].dst_ext, import_filter_list[selection[i].src_idx][selection[i].dst_idx].guid);
          }
        }
      }

      // perform copy
      CopyFilesProgress progress;
      progress.total_size = list_stats.size;
      progress.processed_total_size = progress.copied_total_size = 0;
      QueryPerformanceCounter((PLARGE_INTEGER) &progress.start_time);
      ui.force_update();
      AutoBuffer buffer(g_plugin_options.copy_buf_size);
      for (int i = 0; i < ItemsNumber; i++) {
        copy_files(false, panel_file_list[i].file_dir, file_lists[i], true, dst_dir_path, dst_new_name, stats, progress, options, ui, buffer, log, filters, plugin->session, plugin);
        PanelItem[i].Flags &= ~PPIF_SELECTED;
      }

      // delete source files if moving (only if no errors or skipped files to prevent data loss)
      if (options.move_files && (stats.errors == 0) && (stats.skipped == 0)) {
        DeleteFilesStats del_stats;
        DeleteFilesOptions del_options;
        del_options.ignore_errors = options.ignore_errors;
        del_options.show_stats = options.show_stats;
        del_options.show_error = options.show_error;
        del_options.show_dialog = options.show_dialog;
        DeleteFilesProgress del_progress;
        del_progress.objects = 0;
        del_progress.total_objects = list_stats.files + list_stats.dirs;
        QueryPerformanceCounter((PLARGE_INTEGER) &del_progress.start_time);
        ui.force_update();
        try {
          for (int i = 0; i < ItemsNumber; i++) {
            delete_files(false, panel_file_list[i].file_dir, file_lists[i], del_stats, del_progress, del_options, ui, log, plugin->session, plugin);
          }
        }
        finally (stats.errors += del_stats.errors);
      }

    }
    if (show_dialog && ((options.show_stats == ssoAlways) || ((options.show_stats == ssoIfError) && (stats.errors != 0)))) show_copy_files_results_dlg(stats, log);
    return 1;
  }
  HANDLE_ERROR(0, -1);
}

int WINAPI FAR_EXPORT(DeleteFiles)(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode) {
  SetFileApis set_file_apis;
  if ((ItemsNumber == 0) || (FAR_STRCMP(FAR_FILE_NAME(PanelItem[0].FindData), FAR_T("..")) == 0)) return TRUE;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  bool show_dialog = (OpMode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
  bool show_error = (OpMode & (OPM_FIND | OPM_QUICKVIEW)) == 0;
  try {
    DeleteFilesOptions options;
    options.show_dialog = show_dialog;
    options.show_error = show_error;
    if (show_dialog) {
      options.ignore_errors = g_plugin_options.ignore_errors;
      options.show_stats = g_plugin_options.show_stats;
      if (!show_delete_files_dlg(options)) BREAK;
      if (g_plugin_options.save_def_values) {
        g_plugin_options.ignore_errors = options.ignore_errors;
        g_plugin_options.show_stats = options.show_stats;
        save_def_option_values(g_plugin_options);
      }
    }
    else {
      options.ignore_errors = false;
      options.show_stats = ssoNever;
    }
    DeleteFilesStats stats;
    Log log;
    {
      UiLink ui((OpMode & OPM_SILENT) != 0);
      if (ui.update_needed()) {
        draw_progress_msg(far_get_msg(MSG_PROGRESS_PREPARE));
      }

      UnicodeString dst_dir_path = plugin->current_dir;

      // list of selected files
      FileList panel_file_list;
      panel_items_to_file_list(PanelItem, ItemsNumber, panel_file_list);

      // scan source directories and prepare lists of files to process
      ObjectArray<FileList> file_lists;
      ui.force_update();
      CreateListStats list_stats;
      CreateListOptions list_options;
      list_options.ignore_errors = options.ignore_errors;
      list_options.show_error = options.show_error;
      try {
        for (int i = 0; i < ItemsNumber; i++) {
          file_lists += create_file_list(dst_dir_path, panel_file_list[i].file_name, list_stats, list_options, ui, log, plugin->session, plugin);
        }
      }
      finally (stats.errors = list_stats.errors);

      // init progress data
      DeleteFilesProgress progress;
      progress.objects = 0;
      progress.total_objects = list_stats.files + list_stats.dirs;
      QueryPerformanceCounter((PLARGE_INTEGER) &progress.start_time);
      ui.force_update();
      // delete files
      for (int i = 0; i < ItemsNumber; i++) {
        delete_files(true, dst_dir_path, file_lists[i], stats, progress, options, ui, log, plugin->session, plugin);
        PanelItem[i].Flags &= ~PPIF_SELECTED;
      }
    }
    if (show_dialog && ((options.show_stats == ssoAlways) || ((options.show_stats == ssoIfError) && (stats.errors != 0)))) show_delete_files_results_dlg(stats, log);
    return TRUE;
  }
  HANDLE_ERROR(FALSE, TRUE);
}

int WINAPI FAR_EXPORT(MakeDirectory)(HANDLE hPlugin, FarBuf Name, int OpMode) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  bool show_dialog = (OpMode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
  bool show_error = (OpMode & (OPM_FIND | OPM_QUICKVIEW)) == 0;
  try {
    CreateDirOptions options;
    if (show_dialog) {
      if (!show_create_dir_dlg(options)) BREAK;
      FilePath fp_dir(options.file_name);
      UnicodeString dir;
      if (fp_dir.is_absolute) {
        FilePath fp_cur_dir(plugin->current_dir);
        if ((fp_dir.size() > fp_cur_dir.size()) && fp_dir.equal(0, fp_cur_dir)) dir = fp_dir[fp_cur_dir.size()];
      }
      else {
        dir = fp_dir[0];
      }
      FAR_SET_BUF(Name, dir, plugin->directory_name_buf);
    }
    create_dir(plugin->current_dir, options.file_name, plugin->session, plugin);
    return 1;
  }
  HANDLE_ERROR(0, -1);
}

int WINAPI FAR_EXPORT(Configure)(int ItemNumber) {
  SetFileApis set_file_apis;
  if (show_plugin_options_dlg(g_plugin_options)) {
    save_plugin_options(g_plugin_options);
    return TRUE;
  }
  else return FALSE;
}

int WINAPI FAR_EXPORT(ProcessKey)(HANDLE hPlugin, int Key, unsigned int ControlState) {
  SetFileApis set_file_apis;
  PluginInstance* plugin = (PluginInstance*) hPlugin;
  bool show_error = true;
  try {
    if ((Key == g_plugin_options.key_attr.vcode) && (ControlState == g_plugin_options.key_attr.state)) {
      PanelInfo panel_info;
      if (far_control_ptr(hPlugin, FCTL_GETPANELINFO, &panel_info)) {
        if (panel_info.SelectedItemsNumber != 0) {
          FileAttrOptions options;
          options.single_file = panel_info.SelectedItemsNumber == 1;
          options.attr_and = 0xFFFFFFFF;
          options.attr_or = 0;
          for (int i = 0; i < panel_info.SelectedItemsNumber; i++) {
            PluginPanelItem* ppi = far_get_selected_panel_item(hPlugin, i, panel_info);
            options.attr_and &= ppi->FindData.dwFileAttributes;
            options.attr_or |= ppi->FindData.dwFileAttributes;
          }
          if (show_file_attr_dlg(options)) {
            if ((options.attr_and != 0xFFFFFFFF) || (options.attr_or != 0)) {
              UnicodeString file_name;
              UnicodeString path;
              try {
                PluginPanelItem* ppi = far_get_selected_panel_item(hPlugin, 0, panel_info);
                bool sel_flag = (ppi->Flags & PPIF_SELECTED) != 0;
                for (int i = 0; i < panel_info.ItemsNumber; i++) {
                  PluginPanelItem* ppi = far_get_panel_item(hPlugin, i, panel_info);
                  if ((sel_flag && ((ppi->Flags & PPIF_SELECTED) != 0)) || (!sel_flag && (i == panel_info.CurrentItem))) {
                    file_name = FAR_DECODE_PATH(FAR_FILE_NAME(ppi->FindData));
                    DWORD attr = ppi->FindData.dwFileAttributes & options.attr_and | options.attr_or;
                    COMPOSE_PATH2(path, plugin->current_dir, file_name);
                    set_file_attr(path, attr, plugin->session, plugin);
#ifdef FARAPI17
                    ppi->Flags &= ~PPIF_SELECTED;
#endif
#ifdef FARAPI18
                    g_far.Control(hPlugin, FCTL_SETSELECTION, i, FALSE);
#endif
                  }
                }
              }
#ifdef FARAPI17
              finally (
                g_far.Control(hPlugin, FCTL_SETSELECTION, &panel_info);
                g_far.Control(hPlugin, FCTL_UPDATEPANEL, (void*) 1);
                g_far.Control(hPlugin, FCTL_REDRAWPANEL, NULL);
              );
#endif
#ifdef FARAPI18
              finally (
                far_control_int(hPlugin, FCTL_UPDATEPANEL, 1);
                far_control_ptr(hPlugin, FCTL_REDRAWPANEL, NULL);
              );
#endif
            }
          }
        }
      }
      return TRUE;
    }
    if (((Key == VK_F5) || (Key == VK_F6)) && (ControlState == PKF_SHIFT)) {
      PanelInfo panel_info;
      if (far_control_ptr(hPlugin, FCTL_GETPANELINFO, &panel_info)) {
        if (panel_info.SelectedItemsNumber != 0) {
          PluginPanelItem* ppi = far_get_panel_item(hPlugin, panel_info.CurrentItem, panel_info);
          UnicodeString file_name = FAR_DECODE_PATH(FAR_FILE_NAME(ppi->FindData));
          get_files(hPlugin, ppi, 1, Key == VK_F6, file_name, 0);
          far_control_int(hPlugin, FCTL_UPDATEPANEL, 1);
          far_control_ptr(hPlugin, FCTL_REDRAWPANEL, NULL);
        }
      }
      return TRUE;
    }
    if ((Key == g_plugin_options.key_execute.vcode) && (ControlState == g_plugin_options.key_execute.state)) {
      PanelInfo panel_info;
      if (far_control_ptr(hPlugin, FCTL_GETPANELINFO, &panel_info)) {
        if (panel_info.SelectedItemsNumber != 0) {
          PluginPanelItem* ppi = far_get_panel_item(hPlugin, panel_info.CurrentItem, panel_info);
          UnicodeString file_name = FAR_DECODE_PATH(FAR_FILE_NAME(ppi->FindData));
          UnicodeString cmd_line;
          try {
            cmd_line = get_open_command(plugin->current_dir, file_name, plugin->session);
          }
          catch (Error&) {
            cmd_line = COMPOSE_PATH(plugin->current_dir, file_name);
            if (cmd_line.search(L' ') != -1) cmd_line = L'"' + cmd_line + L'"';
          }
          RunOptions options;
          options.cmd_line = cmd_line;
          if (!show_run_dlg(options)) BREAK;
          if (options.cmd_line.size() == 0) FAIL(CustomError(far_get_msg(MSG_ERR_INVALID_CMD_LINE)));
          if (UNICODE_TO_FARSTR(options.cmd_line) != UNICODE_TO_FARSTR(cmd_line)) {
            cmd_line = options.cmd_line;
          }

          // extract app_name and params
          UnicodeString app_name, params;
          if (cmd_line[0] == L'"') {
            unsigned pos = cmd_line.search(1, L'"');
            if (pos == -1) FAIL(CustomError(far_get_msg(MSG_ERR_INVALID_CMD_LINE)));
            app_name = cmd_line.slice(1, pos - 1).strip();
            params = cmd_line.slice(pos + 1, cmd_line.size() - pos - 1).strip();
          }
          else {
            unsigned pos = cmd_line.search(L' ');
            if (pos == -1) pos = cmd_line.size();
            app_name = cmd_line.slice(0, pos).strip();
            params = cmd_line.slice(pos, cmd_line.size() - pos).strip();
          }

          plugin->last_object = app_name;
          create_process(app_name, params, plugin->session);

          far_control_int(hPlugin, FCTL_UPDATEPANEL, 1);
        }
      }
      return TRUE;
    }
    // toggle 'hide_rom_files' option
    if ((Key == g_plugin_options.key_hide_rom_files.vcode) && (ControlState == g_plugin_options.key_hide_rom_files.state)) {
      g_plugin_options.hide_rom_files = !g_plugin_options.hide_rom_files;
      far_control_int(hPlugin, FCTL_UPDATEPANEL, 1);
      far_control_ptr(hPlugin, FCTL_REDRAWPANEL, NULL);
      return TRUE;
    }
    // open selected directory in Windows Explorer
    if ((Key == VK_RETURN) && (ControlState == PKF_SHIFT)) {
      PanelInfo panel_info;
      if (far_control_ptr(hPlugin, FCTL_GETPANELINFO, &panel_info)) {
        UnicodeString path;
        if (panel_info.SelectedItemsNumber == 0) {
          path = plugin->current_dir;
        }
        else {
          PluginPanelItem* ppi = far_get_panel_item(hPlugin, panel_info.CurrentItem, panel_info);
          const FAR_FIND_DATA& find_data = ppi->FindData;
          if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
            path = COMPOSE_PATH(plugin->current_dir, FAR_DECODE_PATH(FAR_FILE_NAME(find_data)));
          }
          else {
            path = plugin->current_dir;
          }
        }
        UnicodeString cmd_line = L"explorer.exe /root,,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{49BF5420-FA7F-11cf-8011-00A0C90A8F78}\\" + path;
        PROCESS_INFORMATION pi;
        STARTUPINFOW si;
        memset(&si, 0, sizeof(STARTUPINFOW));
        si.cb = sizeof(STARTUPINFOW);
        CHECK_API(CreateProcessW(NULL, (LPWSTR) cmd_line.data(), NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi) != 0);
        VERIFY(CloseHandle(pi.hProcess) != 0);
        VERIFY(CloseHandle(pi.hThread) != 0);
      }
      return TRUE;
    }
    return FALSE;
  }
  HANDLE_ERROR(TRUE, TRUE);
}

void WINAPI FAR_EXPORT(ExitFAR)() {
  SetFileApis set_file_apis;
  cleanup();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    g_h_module = hinstDLL;
  }
  return TRUE;
}
