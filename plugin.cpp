#include <windows.h>

#include <string>
#include <list>
using namespace std;

#include "msg.h"

#include "farutils.hpp"
#include "archive.hpp"

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
  static const wchar_t* plugin_menu[1];

  Info->StructSize = sizeof(PluginInfo);

  plugin_menu[0] = Far::msg_ptr(MSG_PLUGIN_NAME);
  Info->PluginMenuStrings = plugin_menu;
  Info->PluginMenuStringsNumber = ARRAYSIZE(plugin_menu);
}

HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item) {
  ArcLibs arc_libs;
  arc_libs.load(Far::get_plugin_path());
  ArcFormats arc_formats(arc_libs);
  return INVALID_HANDLE_VALUE;
}

HANDLE WINAPI OpenFilePluginW(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode) {
  return INVALID_HANDLE_VALUE;
}

void WINAPI ClosePluginW(HANDLE hPlugin) {

}

void WINAPI GetOpenPluginInfoW(HANDLE hPlugin,struct OpenPluginInfo *Info) {

}

int WINAPI SetDirectoryW(HANDLE hPlugin,const wchar_t *Dir,int OpMode) {
  return FALSE;
}

int WINAPI GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode) {
  return FALSE;
}

void WINAPI FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber) {

}

int WINAPI GetFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode) {
  return 0;
}

int WINAPI PutFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *SrcPath,int OpMode) {
  return 0;
}

int WINAPI DeleteFilesW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode) {
  return FALSE;
}

int WINAPI ProcessKeyW(HANDLE hPlugin,int Key,unsigned int ControlState) {
  return FALSE;
}

int WINAPI ConfigureW(int ItemNumber) {
  return FALSE;
}
