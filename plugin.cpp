#include "msg.h"

#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

ArcLibs libs;
ArcFormats formats;

void load_formats() {
  static bool init_done = false;
  if (!init_done) {
    libs.load(Far::get_plugin_module_path());
    formats.load(libs);
    init_done = true;
  }
}

class Plugin {
private:
  Archive archive;
  wstring current_dir;
  wstring extract_dir;
public:
  Plugin(const wstring& file_path);
  void info(OpenPluginInfo* opi);
  void set_dir(const wstring& dir);
  void list(PluginPanelItem** panel_item, int* items_number);
  void extract(PluginPanelItem* panel_items, int items_number, int move, const wchar_t** dest_path, int op_mode);
};

Plugin::Plugin(const wstring& file_path): archive(formats) {
  load_formats();
  if (!archive.open(file_path))
    FAIL(E_ABORT);
}

void Plugin::info(OpenPluginInfo* opi) {
  opi->StructSize = sizeof(OpenPluginInfo);
  opi->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS;
  opi->CurDir = current_dir.c_str();
}

void Plugin::set_dir(const wstring& dir) {
  wstring new_dir;
  if (dir == L"\\")
    new_dir.assign(dir);
  else if (dir == L"..")
    new_dir = extract_file_path(current_dir);
  else if (dir[0] == L'\\') // absolute path
    new_dir.assign(dir);
  else
    new_dir.assign(L"\\").append(add_trailing_slash(remove_path_root(current_dir))).append(dir);

  if (new_dir == L"\\")
    new_dir.clear();

  archive.find_dir(new_dir);
  current_dir = new_dir;
}

void Plugin::list(PluginPanelItem** panel_items, int* items_number) {
  UInt32 dir_index = archive.find_dir(current_dir);
  FileIndexRange dir_list = archive.get_dir_list(dir_index);
  unsigned size = dir_list.second - dir_list.first;
  PluginPanelItem* items = new PluginPanelItem[size];
  memset(items, 0, size * sizeof(PluginPanelItem));
  try {
    unsigned idx = 0;
    for_each(dir_list.first, dir_list.second, [&] (UInt32 file_index) {
      const FileInfo& file_info = archive.get_file_info(file_index);
      FAR_FIND_DATA& fdata = items[idx].FindData;
      fdata.dwFileAttributes = file_info.attr;
      fdata.ftCreationTime = file_info.ctime;
      fdata.ftLastAccessTime = file_info.atime;
      fdata.ftLastWriteTime = file_info.mtime;
      fdata.nFileSize = file_info.size;
      fdata.nPackSize = file_info.psize;
      fdata.lpwszFileName = file_info.name.c_str();
      items[idx].UserData = file_index;
      idx++;
    });
  }
  catch (...) {
    delete[] items;
    throw;
  }
  *panel_items = items;
  *items_number = size;
}

void Plugin::extract(PluginPanelItem* panel_items, int items_number, int move, const wchar_t** dest_path, int op_mode) {
  ExtractOptions options;
  options.dst_dir = *dest_path;
  options.ignore_errors = false;
  options.overwrite = ooAsk;
  options.move_enabled = archive.updatable();
  options.move_files = move != 0 && options.move_enabled;
  options.show_dialog = (op_mode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
  options.use_tmp_files = (op_mode & (OPM_FIND | OPM_VIEW | OPM_QUICKVIEW)) != 0;
  options.ignore_errors = (op_mode & (OPM_FIND | OPM_QUICKVIEW)) != 0;
  if (!options.show_dialog) {
    options.overwrite = ooOverwrite;
  }
  if (options.show_dialog) {
    if (!extract_dialog(options)) FAIL(E_ABORT);
    if (options.dst_dir != *dest_path) {
      extract_dir = options.dst_dir;
      *dest_path = extract_dir.c_str();
    }
  }
  vector<UInt32> indices;
  UInt32 src_dir_index = archive.find_dir(current_dir);
  if (items_number == 1 && wcscmp(panel_items[0].FindData.lpwszFileName, L"..") == 0) {
    FileIndexRange dir_list = archive.get_dir_list(src_dir_index);
    indices.reserve(dir_list.second - dir_list.first);
    for_each(dir_list.first, dir_list.second, [&] (UInt32 file_index) {
      indices.push_back(file_index);
    });
  }
  else {
    indices.reserve(items_number);
    for (int i = 0; i < items_number; i++) {
      indices.push_back(panel_items[i].UserData);
    }
  }

  ErrorLog error_log;
  archive.extract(src_dir_index, indices, options, error_log);
  if (!error_log.empty()) {
    if (options.show_dialog)
      show_error_log(error_log);
  }
  else {
    if (options.move_files) {
      wstring temp_arc_name = archive.get_temp_file_name();
      try {
        archive.delete_files(indices, temp_arc_name);
        archive.close();
        CHECK_SYS(MoveFileExW(temp_arc_name.c_str(), archive.get_file_name().c_str(), MOVEFILE_REPLACE_EXISTING));
      }
      catch (...) {
        DeleteFileW(temp_arc_name.c_str());
        throw;
      }
      archive.reopen();
    }
  }

  Far::update_panel(this, false);
}

int WINAPI GetMinFarVersion(void) {
  return FARMANAGERVERSION;
}

int WINAPI GetMinFarVersionW(void) {
  return FARMANAGERVERSION;
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
  Far::init(Info);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  FAR_ERROR_HANDLER_BEGIN;
  static const wchar_t* plugin_menu[1];
  Info->StructSize = sizeof(PluginInfo);
  plugin_menu[0] = Far::msg_ptr(MSG_PLUGIN_NAME);
  Info->PluginMenuStrings = plugin_menu;
  Info->PluginMenuStringsNumber = ARRAYSIZE(plugin_menu);
  FAR_ERROR_HANDLER_END(return, return, false);
}

HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item) {
  FAR_ERROR_HANDLER_BEGIN;
  return INVALID_HANDLE_VALUE;
  FAR_ERROR_HANDLER_END(return INVALID_HANDLE_VALUE, return INVALID_HANDLE_VALUE, false);
}

HANDLE WINAPI OpenFilePluginW(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode) {
  FAR_ERROR_HANDLER_BEGIN;
  if (Name == NULL)
    return INVALID_HANDLE_VALUE;
  return new Plugin(Name);
  FAR_ERROR_HANDLER_END(return INVALID_HANDLE_VALUE, return INVALID_HANDLE_VALUE, (OpMode & (OPM_SILENT | OPM_FIND)) != 0);
}

void WINAPI ClosePluginW(HANDLE hPlugin) {
  FAR_ERROR_HANDLER_BEGIN;
  delete reinterpret_cast<Plugin*>(hPlugin);
  FAR_ERROR_HANDLER_END(return, return, true);
}

void WINAPI GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info) {
  FAR_ERROR_HANDLER_BEGIN;
  reinterpret_cast<Plugin*>(hPlugin)->info(Info);
  FAR_ERROR_HANDLER_END(return, return, false);
}

int WINAPI SetDirectoryW(HANDLE hPlugin,const wchar_t *Dir,int OpMode) {
  FAR_ERROR_HANDLER_BEGIN;
  reinterpret_cast<Plugin*>(hPlugin)->set_dir(Dir);
  return TRUE;
  FAR_ERROR_HANDLER_END(return FALSE, return FALSE, (OpMode & (OPM_SILENT | OPM_FIND)) != 0);
}

int WINAPI GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode) {
  FAR_ERROR_HANDLER_BEGIN;
  reinterpret_cast<Plugin*>(hPlugin)->list(pPanelItem, pItemsNumber);
  return TRUE;
  FAR_ERROR_HANDLER_END(return FALSE, return FALSE, (OpMode & (OPM_SILENT | OPM_FIND)) != 0);
}

void WINAPI FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber) {
  FAR_ERROR_HANDLER_BEGIN;
  delete[] PanelItem;
  FAR_ERROR_HANDLER_END(return, return, false);
}

int WINAPI GetFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode) {
  FAR_ERROR_HANDLER_BEGIN
  reinterpret_cast<Plugin*>(hPlugin)->extract(PanelItem, ItemsNumber, Move, DestPath, OpMode);
  return 1;
  FAR_ERROR_HANDLER_END(return 0, return -1, (OpMode & (OPM_FIND | OPM_QUICKVIEW)) != 0);
}

int WINAPI PutFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *SrcPath,int OpMode) {
  FAR_ERROR_HANDLER_BEGIN;
  return 0;
  FAR_ERROR_HANDLER_END(return 0, return -1, (OpMode & OPM_FIND) != 0);
}

int WINAPI DeleteFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
  FAR_ERROR_HANDLER_BEGIN;
  return FALSE;
  FAR_ERROR_HANDLER_END(return FALSE, return FALSE, (OpMode & OPM_SILENT) != 0);
}

int WINAPI ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState) {
  FAR_ERROR_HANDLER_BEGIN;
  return FALSE;
  FAR_ERROR_HANDLER_END(return FALSE, return FALSE, false);
}

int WINAPI ConfigureW(int ItemNumber) {
  FAR_ERROR_HANDLER_BEGIN;
  return FALSE;
  FAR_ERROR_HANDLER_END(return FALSE, return FALSE, false);
}
