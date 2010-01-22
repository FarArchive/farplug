#include "msg.h"

#include "utils.hpp"
#include "sysutils.hpp"
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
  g_options.load();
  Update::init();
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
  Update::execute(true);
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
  Update::clean();
  FAR_ERROR_HANDLER_END(return, return, true);
}

int WINAPI ProcessSynchroEventW(int Event, void *Param) {
  FAR_ERROR_HANDLER_BEGIN;
  if (Event == SE_COMMONSYNCHRO) {
    switch (static_cast<Update::Command>(reinterpret_cast<int>(Param))) {
    case Update::cmdClean:
      Update::clean();
      break;
    case Update::cmdExecute:
      Update::execute(false);
      break;
    }
  }
  return 0;
  FAR_ERROR_HANDLER_END(return 0, return 0, false);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  g_h_instance = hinstDLL;
  return TRUE;
}
