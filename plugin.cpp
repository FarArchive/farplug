#include <vector>
#include <string>
using namespace std;

#include "msg.h"

#include "utils.hpp"
#include "farutils.hpp"
#include "options.hpp"
#include "ui.hpp"
#include "update.hpp"

int WINAPI GetMinFarVersion(void) {
  return FARMANAGERVERSION;
}

int WINAPI GetMinFarVersionW(void) {
  return FARMANAGERVERSION;
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
  Far::init(Info);
  FAR_ERROR_HANDLER_BEGIN;
  Updater::initialize();
  FAR_ERROR_HANDLER_END(return, return, false);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  FAR_ERROR_HANDLER_BEGIN;
  static const wchar_t* plugin_menu[1];
  static const wchar_t* config_menu[1];

  Info->StructSize = sizeof(PluginInfo);

  Info->Flags = PF_PRELOAD;

  plugin_menu[0] = Far::msg_ptr(MSG_PLUGIN_NAME);
  Info->PluginMenuStrings = plugin_menu;
  Info->PluginMenuStringsNumber = ARRAYSIZE(plugin_menu);
  config_menu[0] = Far::msg_ptr(MSG_PLUGIN_NAME);
  Info->PluginConfigStrings = config_menu;
  Info->PluginConfigStringsNumber = ARRAYSIZE(config_menu);
  FAR_ERROR_HANDLER_END(return, return, false);
}

HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item) {
  FAR_ERROR_HANDLER_BEGIN;
  if (Updater::check())
    Updater::execute();
  else
    info_dlg(Far::get_msg(MSG_UPDATE_NO_NEW_VERSION));
  return INVALID_HANDLE_VALUE;
  FAR_ERROR_HANDLER_END(return INVALID_HANDLE_VALUE, return INVALID_HANDLE_VALUE, false);
}

int WINAPI ConfigureW(int ItemNumber) {
  FAR_ERROR_HANDLER_BEGIN;
  if (config_dialog(g_options)) {
    g_options.save();
    return TRUE;
  }
  return FALSE;
  FAR_ERROR_HANDLER_END(return FALSE, return FALSE, false);
}

void WINAPI ExitFARW(void) {
  FAR_ERROR_HANDLER_BEGIN;
  Updater::finalize();
  FAR_ERROR_HANDLER_END(return, return, true);
}

int WINAPI ProcessSynchroEventW(int Event, void *Param) {
  FAR_ERROR_HANDLER_BEGIN;
  if (Event == SE_COMMONSYNCHRO) {
    Updater::execute();
  }
  return 0;
  FAR_ERROR_HANDLER_END(return 0, return 0, false);
}
