#include <windows.h>
#include <msiquery.h>
#include <imagehlp.h>

#include <algorithm>
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
  UPDATE_FEATURE("ftpDirList", "FExcept");
  UPDATE_FEATURE("ftpNotify", "FExcept");
  UPDATE_FEATURE("ftpProgress", "FExcept");
  UPDATE_FEATURE("HlfViewer", "FExcept");
  UPDATE_FEATURE("MacroView", "FExcept");
  UPDATE_FEATURE("MultiArc", "FExcept");
  UPDATE_FEATURE("Ace", "FExcept");
  UPDATE_FEATURE("Arc", "FExcept");
  UPDATE_FEATURE("Arj", "FExcept");
  UPDATE_FEATURE("Cab", "FExcept");
  UPDATE_FEATURE("Custom", "FExcept");
  UPDATE_FEATURE("Ha", "FExcept");
  UPDATE_FEATURE("Lzh", "FExcept");
  UPDATE_FEATURE("Rar", "FExcept");
  UPDATE_FEATURE("TarGz", "FExcept");
  UPDATE_FEATURE("Zip", "FExcept");
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
#ifdef bundle
  UPDATE_FEATURE("_7_Zip", "Russian");
  UPDATE_FEATURE("_7_Zip", "FExcept");
  UPDATE_FEATURE("AltHistory", "Russian");
  UPDATE_FEATURE("Colorer", "Russian");
  UPDATE_FEATURE("Colorer", "FExcept");
  UPDATE_FEATURE("FarSvc", "Russian");
  UPDATE_FEATURE("FarTrans", "Russian");
  UPDATE_FEATURE("FileCopyEx", "Russian");
  UPDATE_FEATURE("FileCopyEx", "FExcept");
  UPDATE_FEATURE("ntfsfile", "Russian");
  UPDATE_FEATURE("ntfsfile", "FExcept");
  UPDATE_FEATURE("Registry", "Russian");
  UPDATE_FEATURE("RESearch", "Russian");
  UPDATE_FEATURE("UnInstall", "Russian");
  UPDATE_FEATURE("UnInstall", "FExcept");
  UPDATE_FEATURE("wmexplorer", "Russian");
  UPDATE_FEATURE("wmexplorer", "FExcept");
  UPDATE_FEATURE("FarHints", "Russian");
  UPDATE_FEATURE("FarHintsCursors", "Russian");
  UPDATE_FEATURE("FarHintsFolders", "Russian");
  UPDATE_FEATURE("FarHintsImage", "Russian");
  UPDATE_FEATURE("FarHintsMP3", "Russian");
  UPDATE_FEATURE("FarHintsProcess", "Russian");
  UPDATE_FEATURE("FarHintsVerInfo", "Russian");
  UPDATE_FEATURE("FontMan", "Russian");
  UPDATE_FEATURE("qPlayEx", "Russian");
  UPDATE_FEATURE("UCharMap", "Russian");
  UPDATE_FEATURE("yac", "Russian");
  UPDATE_FEATURE("airbrush", "FExcept");
  UPDATE_FEATURE("armgnuasm.fmt", "FExcept");
  UPDATE_FEATURE("awk.fmt", "FExcept");
  UPDATE_FEATURE("c.fmt", "FExcept");
  UPDATE_FEATURE("html.fmt", "FExcept");
  UPDATE_FEATURE("pas.fmt", "FExcept");
  UPDATE_FEATURE("php.fmt", "FExcept");
  UPDATE_FEATURE("re2c.fmt", "FExcept");
  UPDATE_FEATURE("sql.fmt", "FExcept");
  UPDATE_FEATURE("yacclex.fmt", "FExcept");
  UPDATE_FEATURE("zcustom.fmt", "FExcept");
  UPDATE_FEATURE("bcopy", "Russian");
  UPDATE_FEATURE("bcopy", "FExcept");
  UPDATE_FEATURE("calc", "Russian");
  UPDATE_FEATURE("dialogtools", "FExcept");
  UPDATE_FEATURE("EditCmpl", "Russian");
  UPDATE_FEATURE("EditCmpl", "FExcept");
  UPDATE_FEATURE("esc", "Russian");
  if (MsiSetFeatureState(h_install, "escUkrainian", INSTALLSTATE_ABSENT) != ERROR_SUCCESS) return ERROR_INSTALL_FAILURE;
  UPDATE_FEATURE("esc", "FExcept");
  UPDATE_FEATURE("esc_tsc_mini", "FExcept");
  UPDATE_FEATURE("farspell", "Russian");
  UPDATE_FEATURE("farspell", "FExcept");
  UPDATE_FEATURE("MailView", "Russian");
  UPDATE_FEATURE("MailView", "FExcept");
  UPDATE_FEATURE("Folder.mvp", "FExcept");
  UPDATE_FEATURE("oe_dbx.mvp", "FExcept");
  UPDATE_FEATURE("pkt.mvp", "FExcept");
  UPDATE_FEATURE("Squish.mvp", "FExcept");
  UPDATE_FEATURE("TheBat.mvp", "FExcept");
  UPDATE_FEATURE("Unix.mvp", "FExcept");
  UPDATE_FEATURE("ntevent", "Russian");
  UPDATE_FEATURE("ntevent", "FExcept");
  UPDATE_FEATURE("userman", "FExcept");
  UPDATE_FEATURE("VisRen", "Russian");
  UPDATE_FEATURE("VisRen", "FExcept");
  UPDATE_FEATURE("Visualizer", "FExcept");
  UPDATE_FEATURE("true_tpl", "Russian");
  UPDATE_FEATURE("true_tpl", "FExcept");
  UPDATE_FEATURE("NewArc7Zip", "FExcept");
  UPDATE_FEATURE("NewArcAce", "FExcept");
  UPDATE_FEATURE("NewArcMA", "FExcept");
  UPDATE_FEATURE("NewArcMAAce", "FExcept");
  UPDATE_FEATURE("NewArcMAArc", "FExcept");
  UPDATE_FEATURE("NewArcMAArj", "FExcept");
  UPDATE_FEATURE("NewArcMACab", "FExcept");
  UPDATE_FEATURE("NewArcMACustom", "FExcept");
  UPDATE_FEATURE("NewArcMAHa", "FExcept");
  UPDATE_FEATURE("NewArcMALzh", "FExcept");
  UPDATE_FEATURE("NewArcMARar", "FExcept");
  UPDATE_FEATURE("NewArcMATarGz", "FExcept");
  UPDATE_FEATURE("NewArcMAZip", "FExcept");
  UPDATE_FEATURE("NewArcRar", "FExcept");
  UPDATE_FEATURE("NewArcTarGz", "FExcept");
  UPDATE_FEATURE("NewArcZip", "FExcept");
  UPDATE_FEATURE("NewArcWcx", "FExcept");
  UPDATE_FEATURE("NewArc", "Russian");
  UPDATE_FEATURE("NewArc", "FExcept");
  UPDATE_FEATURE("NewArc", "Changelogs");
  UPDATE_FEATURE("pictureview", "Russian");
#endif
  return ERROR_SUCCESS;
}

const char* exclude_list[] = {
  "bass.dll",
  "bass_aac.inp",
  "bass_ac3.inp",
  "bass_alac.inp",
  "bass_ape.inp",
  "bass_mpc.inp",
  "basscd.inp",
  "bassflac.inp",
  "bassmidi.inp",
  "basswma.inp",
  "basswv.inp",
  "tags.dll",
};

bool cmp(const char* s1, const char* s2) {
  return stricmp(s1, s2) < 0;
}

void gen_dll_list(const string& path, list<string>& dll_list, list<string>& exe_list) {
  WIN32_FIND_DATA find_data;
  HANDLE h_find = FindFirstFile((path + "\\*").c_str(), &find_data);
  if (h_find == INVALID_HANDLE_VALUE) return;
  while (true) {
    if ((strcmp(find_data.cFileName, ".") != 0) && (strcmp(find_data.cFileName, "..") != 0)) {
      string file_name = path + '\\' + find_data.cFileName;
      if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        gen_dll_list(file_name, dll_list, exe_list);
      }
      else {
        if (!binary_search(exclude_list, exclude_list + ARRAYSIZE(exclude_list), find_data.cFileName, cmp)) {
          LOADED_IMAGE* image = ImageLoad(file_name.c_str(), "");
          if (image) {
            if (!image->fSystemImage && !image->fDOSImage) {
              if (image->Characteristics & IMAGE_FILE_DLL) dll_list.push_back(file_name);
              else exe_list.push_back(file_name);
            }
            ImageUnload(image);
          }
        }
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
  if (install_dir[install_dir_size - 1] == '\\') install_dir[install_dir_size - 1] = 0;

  list<string> dll_list, exe_list;
  gen_dll_list(install_dir, dll_list, exe_list);

  PMSIHANDLE h_progress_rec = MsiCreateRecord(4);
  MsiRecordSetInteger(h_progress_rec, 1, 0);
  MsiRecordSetInteger(h_progress_rec, 2, dll_list.size() * 2 + exe_list.size());
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
  for (list<string>::const_iterator file_name = exe_list.begin(); file_name != exe_list.end(); file_name++) {
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
