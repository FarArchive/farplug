#include <windows.h>

#include "plugin.hpp"

int WINAPI GetMinFarVersion(void) {
  return FARMANAGERVERSION;
}

int WINAPI GetMinFarVersionW(void) {
  return FARMANAGERVERSION;
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {

}

void WINAPI GetPluginInfoW(struct PluginInfo *Info) {

}

HANDLE WINAPI OpenPluginW(int OpenFrom,INT_PTR Item) {
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
