#include <windows.h>
#include <msiquery.h>

bool is_inst(MSIHANDLE h_install, const char* name) {
  INSTALLSTATE st_inst, st_action;
  MsiGetFeatureState(h_install, name, &st_inst, &st_action);
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
  return ERROR_SUCCESS;
}
