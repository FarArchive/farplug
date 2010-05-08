#include "msg.h"

#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
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

Plugin::Plugin(const wstring& file_path): archive(formats, file_path) {
  load_formats();
  if (!archive.open())
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

  archive.dir_find(new_dir);
  current_dir = new_dir;
}

void Plugin::list(PluginPanelItem** panel_items, int* items_number) {
  UInt32 dir_index = archive.dir_find(current_dir);
  FileListRef fl_ref = archive.dir_list(dir_index);
  unsigned size = fl_ref.second - fl_ref.first;
  PluginPanelItem* items = new PluginPanelItem[size];
  memset(items, 0, size * sizeof(PluginPanelItem));
  try {
    FileList::const_iterator& file_info = fl_ref.first;
    for (unsigned i = 0; i < size; i++, file_info++) {
      FAR_FIND_DATA& fdata = items[i].FindData;
      fdata.dwFileAttributes = file_info->attr;
      fdata.ftCreationTime = file_info->ctime;
      fdata.ftLastAccessTime = file_info->atime;
      fdata.ftLastWriteTime = file_info->mtime;
      fdata.nFileSize = file_info->size;
      fdata.nPackSize = file_info->psize;
      fdata.lpwszFileName = file_info->name.c_str();
      items[i].UserData = file_info->index;
    }
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
  options.move_files = move != 0;
  options.show_dialog = (op_mode & (OPM_SILENT | OPM_FIND | OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) == 0;
  options.use_tmp_files = (op_mode & (OPM_FIND | OPM_VIEW | OPM_QUICKVIEW)) != 0;
  if (!options.show_dialog) {
    options.ignore_errors = true;
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
  UInt32 src_dir_index = archive.dir_find(current_dir);
  if (items_number == 1 && wcscmp(panel_items[0].FindData.lpwszFileName, L"..") == 0) {
    FileListRef fl = archive.dir_list(src_dir_index);
    indices.reserve(fl.second - fl.first);
    for (FileList::const_iterator file_info = fl.first; file_info != fl.second; file_info++) {
      indices.push_back(file_info->index);
    }
  }
  else {
    indices.reserve(items_number);
    for (int i = 0; i < items_number; i++) {
      indices.push_back(panel_items[i].UserData);
    }
  }
  archive.extract(src_dir_index, indices, options);
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
