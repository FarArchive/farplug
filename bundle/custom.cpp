#include <windows.h>
#include <msiquery.h>
#include <imagehlp.h>

#include <list>
#include <string>
using namespace std;

bool is_inst(MSIHANDLE h_install, const char* name) {
  INSTALLSTATE st_inst, st_action;
  if (MsiGetFeatureState(h_install, name, &st_inst, &st_action) != ERROR_SUCCESS) return false;
  INSTALLSTATE st = st_action;
  if (st <= 0) st = st_inst;
  if (st <= 0) return false;
  if ((st == INSTALLSTATE_REMOVED) || (st == INSTALLSTATE_ABSENT)) return false;
  return true;
}

#define UPDATE_FEATURE(plugin, feature) if (MsiSetFeatureState(h_install, plugin##feature, is_inst(h_install, plugin) && is_inst(h_install, feature) ? INSTALLSTATE_LOCAL : INSTALLSTATE_ABSENT) != ERROR_SUCCESS) return ERROR_INSTALL_FAILURE

UINT __stdcall UpdateFeatureState(MSIHANDLE h_install) {
  UPDATE_FEATURE("Align", "Russian");
  UPDATE_FEATURE("AutoWrap", "Russian");
  UPDATE_FEATURE("Brackets", "Russian");
  UPDATE_FEATURE("DrawLine", "Russian");
  UPDATE_FEATURE("EditCase", "Russian");
  UPDATE_FEATURE("Compare", "Russian");
  UPDATE_FEATURE("EMenu", "Russian");
  UPDATE_FEATURE("FARCmds", "Russian");
  UPDATE_FEATURE("FileCase", "Russian");
  UPDATE_FEATURE("FarFtp", "Russian");
  UPDATE_FEATURE("HlfViewer", "Russian");
  UPDATE_FEATURE("MacroView", "Russian");
  UPDATE_FEATURE("MultiArc", "Russian");
  UPDATE_FEATURE("Network", "Russian");
  UPDATE_FEATURE("Proclist", "Russian");
  UPDATE_FEATURE("TmpPanel", "Russian");
  UPDATE_FEATURE("Align", "FExcept");
  UPDATE_FEATURE("AutoWrap", "FExcept");
  UPDATE_FEATURE("Brackets", "FExcept");
  UPDATE_FEATURE("DrawLine", "FExcept");
  UPDATE_FEATURE("EditCase", "FExcept");
  UPDATE_FEATURE("Compare", "FExcept");
  UPDATE_FEATURE("EMenu", "FExcept");
  UPDATE_FEATURE("FARCmds", "FExcept");
  UPDATE_FEATURE("FileCase", "FExcept");
  UPDATE_FEATURE("FarFtp", "FExcept");
  UPDATE_FEATURE("HlfViewer", "FExcept");
  UPDATE_FEATURE("MacroView", "FExcept");
  UPDATE_FEATURE("MultiArc", "FExcept");
  UPDATE_FEATURE("Network", "FExcept");
  UPDATE_FEATURE("Proclist", "FExcept");
  UPDATE_FEATURE("TmpPanel", "FExcept");
  UPDATE_FEATURE("Docs", "Russian");
  UPDATE_FEATURE("Docs", "FExcept");
  UPDATE_FEATURE("Changelogs", "FExcept");
  UPDATE_FEATURE("Align", "Changelogs");
  UPDATE_FEATURE("AutoWrap", "Changelogs");
  UPDATE_FEATURE("Brackets", "Changelogs");
  UPDATE_FEATURE("DrawLine", "Changelogs");
  UPDATE_FEATURE("EditCase", "Changelogs");
  UPDATE_FEATURE("Compare", "Changelogs");
  UPDATE_FEATURE("EMenu", "Changelogs");
  UPDATE_FEATURE("FARCmds", "Changelogs");
  UPDATE_FEATURE("FileCase", "Changelogs");
  UPDATE_FEATURE("FarFtp", "Changelogs");
  UPDATE_FEATURE("HlfViewer", "Changelogs");
  UPDATE_FEATURE("MacroView", "Changelogs");
  UPDATE_FEATURE("MultiArc", "Changelogs");
  UPDATE_FEATURE("Network", "Changelogs");
  UPDATE_FEATURE("Proclist", "Changelogs");
  UPDATE_FEATURE("TmpPanel", "Changelogs");
  UPDATE_FEATURE("_7_Zip", "Russian");
  UPDATE_FEATURE("_7_Zip", "FExcept");
  UPDATE_FEATURE("AltHistory", "Russian");
  UPDATE_FEATURE("Colorer", "Russian");
  UPDATE_FEATURE("FarSvc", "Russian");
  UPDATE_FEATURE("FarTrans", "Russian");
  UPDATE_FEATURE("FileCopyEx", "Russian");
  UPDATE_FEATURE("FileCopyEx", "FExcept");
  UPDATE_FEATURE("ntfsfile", "Russian");
  UPDATE_FEATURE("ntfsfile", "FExcept");
  UPDATE_FEATURE("PEDITOR", "Russian");
  UPDATE_FEATURE("Registry", "Russian");
//  UPDATE_FEATURE("Registry", "Italian");
  UPDATE_FEATURE("RESearch", "Russian");
  UPDATE_FEATURE("Resource", "Russian");
  UPDATE_FEATURE("UnInstall", "Russian");
  UPDATE_FEATURE("UnInstall", "FExcept");
  UPDATE_FEATURE("wmexplorer", "Russian");
  UPDATE_FEATURE("wmexplorer", "FExcept");
  return ERROR_SUCCESS;
}


bool check_dll(const string& file_name) {
  LOADED_IMAGE* image = ImageLoad(file_name.c_str(), "");
  bool res = (image != NULL) && (image->Characteristics & IMAGE_FILE_DLL);
  if (image) ImageUnload(image);
  return res;
}

void gen_dll_list(const string& path, list<string>& dll_list) {
  WIN32_FIND_DATA find_data;
  HANDLE h_find = FindFirstFile((path + "\\*").c_str(), &find_data);
  if (h_find == INVALID_HANDLE_VALUE) return;
  while (true) {
    if ((strcmp(find_data.cFileName, ".") != 0) && (strcmp(find_data.cFileName, "..") != 0)) {
      string file_name = path + '\\' + find_data.cFileName;
      if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        gen_dll_list(file_name, dll_list);
      }
      else {
        if (check_dll(file_name)) dll_list.push_back(file_name);
      }
    }
    if (!FindNextFile(h_find, &find_data)) break;
  }
  FindClose(h_find);
}

const ULONG64 c_start_base = 0x20000000;

UINT __stdcall Optimize(MSIHANDLE h_install) {
  if (!is_inst(h_install, "Far")) return ERROR_SUCCESS;

  PMSIHANDLE h_action_start_rec = MsiCreateRecord(3);
  MsiRecordSetString(h_action_start_rec, 1, "Optimize");
  MsiRecordSetString(h_action_start_rec, 2, "Optimizing performance");
  MsiRecordSetString(h_action_start_rec, 3, "[1] [2]: [3]");
  MsiProcessMessage(h_install, INSTALLMESSAGE_ACTIONSTART, h_action_start_rec);

  char install_dir[MAX_PATH];
  DWORD install_dir_size = MAX_PATH;
  if (MsiGetProperty(h_install, "INSTALLDIR", install_dir, &install_dir_size) != ERROR_SUCCESS) return ERROR_INSTALL_FAILURE;
  string plugin_dir(install_dir);
  if (plugin_dir[plugin_dir.size() - 1] != '\\') plugin_dir += '\\';
  plugin_dir += "plugins";

  list<string> dll_list;
  gen_dll_list(plugin_dir, dll_list);

  PMSIHANDLE h_progress_rec = MsiCreateRecord(4);
  MsiRecordSetInteger(h_progress_rec, 1, 0);
  MsiRecordSetInteger(h_progress_rec, 2, dll_list.size() * 2);
  MsiRecordSetInteger(h_progress_rec, 3, 0);
  MsiRecordSetInteger(h_progress_rec, 4, 0);
  MsiProcessMessage(h_install, INSTALLMESSAGE_PROGRESS, h_progress_rec);
  MsiRecordSetInteger(h_progress_rec, 1, 1);
  MsiRecordSetInteger(h_progress_rec, 2, 1);
  MsiRecordSetInteger(h_progress_rec, 3, 1);
  MsiProcessMessage(h_install, INSTALLMESSAGE_PROGRESS, h_progress_rec);

  PMSIHANDLE h_action_rec = MsiCreateRecord(3);
  MsiRecordSetString(h_action_rec, 1, "rebase");
  ULONG64 curr_base = c_start_base;
  for (list<string>::const_iterator file_name = dll_list.begin(); file_name != dll_list.end(); file_name++) {
    MsiRecordSetString(h_action_rec, 2, file_name->c_str());
    ULONG64 old_base, new_base = curr_base;
    ULONG old_size, new_size;
    if (ReBaseImage64(file_name->c_str(), "", TRUE, FALSE, FALSE, 0, &old_size, &old_base, &new_size, &new_base, 0)) {
      curr_base = new_base;
      MsiRecordSetString(h_action_rec, 3, "ok");
    }
    else {
      MsiRecordSetInteger(h_action_rec, 3, GetLastError());
    }
    MsiProcessMessage(h_install, INSTALLMESSAGE_ACTIONDATA, h_action_rec);
  }

  MsiRecordSetString(h_action_rec, 1, "bind");
  for (list<string>::const_iterator file_name = dll_list.begin(); file_name != dll_list.end(); file_name++) {
    MsiRecordSetString(h_action_rec, 2, file_name->c_str());
    if (BindImage(file_name->c_str(), "", "")) {
      MsiRecordSetString(h_action_rec, 3, "ok");
    }
    else {
      MsiRecordSetInteger(h_action_rec, 3, GetLastError());
    }
    MsiProcessMessage(h_install, INSTALLMESSAGE_ACTIONDATA, h_action_rec);
  }

  return ERROR_SUCCESS;
}
