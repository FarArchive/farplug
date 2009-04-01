//дополнительные описания к ключам
const TCHAR *HelpTopics[] =
{
  _T("DisplayName"),
  _T("InstallLocation"),
  _T("UninstallString"),
  _T("Publisher"),
  _T("URLInfoAbout"),
  _T("URLUpdateInfo"),
  _T("Comments"),
  _T("DisplayVersion"),
  _T("InstallDate")
};

enum
{
  LIST_BOX,
  DMU_UPDATE = DM_USER+1
};

enum
{
  DisplayName,
  InstallLocation,
  UninstallString,
  Publisher,
  URLInfoAbout,
  URLUpdateInfo,
  Comments,
  DisplayVersion,
  InstallDate
};

#define sizeofa(array) (sizeof(array)/sizeof(array[0]))
const int KeysCount = sizeofa(HelpTopics);
struct RegKeyPath {
  HKEY Root;
  const TCHAR* Path;
} UninstKeys[] = {
  { HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
  { HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
  { HKEY_CURRENT_USER, _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
  { HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
};
int nCount;
FarList FL;
FarListItem* FLI = NULL;
int ListSize;
HANDLE hStdout;
CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

struct KeyInfo
{
  TCHAR Keys[KeysCount][MAX_PATH];
#ifdef FARAPI18
  TCHAR ListItem[MAX_PATH];
#endif
  bool Avail[KeysCount];
  RegKeyPath RegKey;
  TCHAR SubKeyName[MAX_PATH];
  bool WindowsInstaller;
} *p = NULL;

bool ValidGuid(const TCHAR* guid) {
  const unsigned c_max_guid_len = 38;
  wchar_t buf[c_max_guid_len + 1];
  ZeroMemory(buf, sizeof(buf));
  unsigned l = lstrlen(guid);
  for (unsigned i = 0; (i < c_max_guid_len) && (i < l); i++) buf[i] = guid[i];
  IID iid;
  return IIDFromString(buf, &iid) == S_OK;
}

//чтение реестра
bool FillReg(KeyInfo & key, TCHAR * Buf, RegKeyPath & RegKey)
{
  HKEY userKey;
  DWORD regType;
  TCHAR fullN[MAX_PATH];
  LONG ExitCode;
  DWORD bufSize;

  key.RegKey = RegKey;
  lstrcpy(key.SubKeyName, Buf);
  lstrcpy(fullN,key.RegKey.Path);
  lstrcat(fullN,_T("\\"));
  lstrcat(fullN,key.SubKeyName);
  if (RegOpenKeyEx(key.RegKey.Root, fullN, 0, KEY_READ, &userKey) != ERROR_SUCCESS)
    return FALSE;
  key.WindowsInstaller = (RegQueryValueEx(userKey,_T("WindowsInstaller"),0,NULL,NULL,NULL) == ERROR_SUCCESS) && ValidGuid(key.SubKeyName);
  for (int i=0;i<KeysCount;i++)
  {
    bufSize = MAX_PATH * sizeof(TCHAR);
    ExitCode = RegQueryValueEx(userKey,HelpTopics[i],0,&regType,(LPBYTE)key.Keys[i],&bufSize);
    if (ExitCode != ERROR_SUCCESS || lstrcmp(key.Keys[i],_T("")) == 0)
    {
      if ((i == UninstallString && !key.WindowsInstaller) || i == DisplayName)
      {
        RegCloseKey(userKey);
        return FALSE;
      }
      lstrcpy(key.Keys[i],_T(""));
      key.Avail[i] = FALSE;
    }
    else
      key.Avail[i] = TRUE;
  }
  RegCloseKey(userKey);
  return TRUE;
}

//заполнение пункта диалога
void FillDialog(FarDialogItem & DialogItem, int Type, int X1, int Y1, int X2, int Y2,
                int Flags, int nData)
{
  const TCHAR* s = nData != -1 ? GetMsg(nData) : _T("");
#ifdef FARAPI17
  lstrcpy(DialogItem.Data, s);
#endif
#ifdef FARAPI18
  DialogItem.PtrData = s;
#endif

  DialogItem.X1 = X1;
  DialogItem.X2 = X2;
  DialogItem.Y1 = Y1;
  DialogItem.Y2 = Y2;

  DialogItem.Flags = Flags;
  DialogItem.Type = Type;
  DialogItem.Selected = 0;
  DialogItem.DefaultButton = 0;
  DialogItem.Focus = 0;

  if (Type == DI_BUTTON)
  {
    DialogItem.DefaultButton = 1;
    DialogItem.Focus = 1;
  }
}

void DisplayEntry(int Sel)
{
  unsigned sx = 70;

  unsigned max_len = 0;
  unsigned cnt = 0;
  for (int i=0;i<KeysCount;i++) {
    if (p[Sel].Avail[i]) {
      unsigned len = lstrlen(p[Sel].Keys[i]);
      if (len + 3 > sx) sx = len + 3;
      cnt++;
    }
  }

  unsigned con_sx = 80;
  HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
  if (con != INVALID_HANDLE_VALUE) {
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    if (GetConsoleScreenBufferInfo(con, &con_info)) {
      con_sx = con_info.srWindow.Right - con_info.srWindow.Left + 1;
    }
  }

  if (sx + 10 > con_sx) sx = con_sx - 10;
  unsigned sy = cnt * 2;

  unsigned di_cnt = cnt * 2 + 2;
  FarDialogItem* DialogItems = new FarDialogItem[di_cnt];
  unsigned y = 2;
  unsigned idx = 1;
  for (int i=0;i<KeysCount;i++) {
    if (p[Sel].Avail[i]) {
      FillDialog(DialogItems[idx], DI_TEXT, 5, y, 0, y, 0, MName + i);
      idx++;
      y++;
      FillDialog(DialogItems[idx], DI_EDIT, 5, y, sx + 2, y, DIF_READONLY, -1);
      y++;
#ifdef FARAPI17
      if (lstrlen(p[Sel].Keys[i]) < ARRAYSIZE(DialogItems[idx].Data)) lstrcpy(DialogItems[idx].Data, p[Sel].Keys[i]);
      else lstrcpyn(DialogItems[idx].Data, p[Sel].Keys[i], ARRAYSIZE(DialogItems[idx].Data));
      if (i != DisplayName && i != UninstallString) //DisplayName, UninstallString у нас и так в кодировке OEM
        CharToOem(DialogItems[idx].Data, DialogItems[idx].Data); //Переводим в OEM кодировку
#endif
#ifdef FARAPI18
      DialogItems[idx].PtrData = p[Sel].Keys[i];
#endif
      idx++;
    }
  }
  FillDialog(DialogItems[0], DI_DOUBLEBOX, 3, 1, sx + 4, sy + 2, 0, MUninstallEntry);
#ifdef FARAPI17
  Info.Dialog(Info.ModuleNumber, -1, -1, sx + 8, sy + 4, NULL, DialogItems, di_cnt);
#endif
#ifdef FARAPI18
  HANDLE h_dlg = Info.DialogInit(Info.ModuleNumber, -1, -1, sx + 8, sy + 4, NULL, DialogItems, di_cnt, 0, 0, NULL, 0);
  if (h_dlg != INVALID_HANDLE_VALUE) {
    Info.DialogRun(h_dlg);
    Info.DialogFree(h_dlg);
  }
#endif
  delete[] DialogItems;
}

bool ExecuteEntry(int Sel, bool WaitEnd)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  HANDLE hScreen; //for SaveScreen/RestoreScreen

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  TCHAR cmd_line[MAX_PATH];
  if (p[Sel].WindowsInstaller) {
    lstrcpy(cmd_line, _T("msiexec /x"));
    lstrcat(cmd_line, p[Sel].SubKeyName);
  }
  else lstrcpy(cmd_line, p[Sel].Keys[UninstallString]);

  hScreen = Info.SaveScreen(0,0,-1,-1); //Это необходимо сделать, т.к. после запущенных программ нужно обновить окно ФАРа
  BOOL ifCreate = CreateProcess   // Start the child process.
  (
    NULL,                         // No module name (use command line).
    cmd_line,                     // Command line.
    NULL,                         // Process handle not inheritable.
    NULL,                         // Thread handle not inheritable.
    FALSE,                        // Set handle inheritance to FALSE.
    0,                            // No creation flags.
    NULL,                         // Use parent's environment block.
    NULL,                         // Use parent's starting directory.
    &si,                          // Pointer to STARTUPINFO structure.
    &pi                           // Pointer to PROCESS_INFORMATION structure.
  );
  if (!ifCreate) //not Create
  {
    if (hScreen)
      Info.RestoreScreen(hScreen);
    DrawMessage(FMSG_WARNING, 1, "%s",GetMsg(MPlugIn),GetMsg(MRunProgErr),GetMsg(MOK),NULL);
    return FALSE;
  }

  TCHAR SaveTitle[MAX_PATH];
  GetConsoleTitle(SaveTitle,sizeof(SaveTitle));
  SetConsoleTitle(cmd_line);

  if (WaitEnd) // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);
  // Close process and thread handles.
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  SetConsoleTitle(SaveTitle);

  if (hScreen)
  {
    Info.RestoreScreen(NULL);
    Info.RestoreScreen(hScreen);
  }
  return TRUE;
}

void DeleteEntry(int Sel)
{
  HKEY userKey;
  RegOpenKeyEx(p[Sel].RegKey.Root, p[Sel].RegKey.Path, 0, KEY_SET_VALUE, &userKey);
  if (RegDeleteKey(userKey, p[Sel].SubKeyName) != ERROR_SUCCESS)
    DrawMessage(FMSG_WARNING, 1, "%s",GetMsg(MPlugIn),GetMsg(MDelRegErr),GetMsg(MOK),NULL);
  RegCloseKey(userKey);
}

//сравнить строки
int __cdecl CompareEntries(const void* item1, const void* item2)
{
  return FSF.LStricmp(reinterpret_cast<const KeyInfo*>(item1)->Keys[DisplayName], reinterpret_cast<const KeyInfo*>(item2)->Keys[DisplayName]);
}

#define EMPTYSTR _T("  ")
#define JUMPREALLOC 50
//Обновление информации
void UpDateInfo(void)
{
  DWORD fEnumIndex;
  FILETIME ftLastWrite;
  DWORD bufSize;
  HKEY hKey;
  TCHAR Buf[MAX_PATH];
  DWORD cSubKeys;
  int nRealCount = 0;

  nCount = 0;
  for (int i=0;i<sizeofa(UninstKeys);i++)
  {
    if (RegOpenKeyEx(UninstKeys[i].Root, UninstKeys[i].Path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
      continue;
    if (RegQueryInfoKey(hKey,NULL,NULL,NULL,&cSubKeys,NULL,NULL,NULL,NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
      continue;

    for (fEnumIndex=0, bufSize = sizeof(Buf); fEnumIndex<cSubKeys; fEnumIndex++, bufSize = sizeof(Buf))
    {
      if (RegEnumKeyEx(hKey, fEnumIndex, Buf, &bufSize, NULL, NULL, NULL, &ftLastWrite) != ERROR_SUCCESS)
        continue;
      if (nCount+1 > nRealCount)
      {
        nRealCount += JUMPREALLOC;
        p = (KeyInfo *) realloc(p, sizeof(KeyInfo) * nRealCount);
      }
      if (FillReg(p[nCount], Buf, UninstKeys[i]))
      {
#ifdef FARAPI17
        CharToOem(p[nCount].Keys[DisplayName], p[nCount].Keys[DisplayName]);
        CharToOem(p[nCount].Keys[UninstallString], p[nCount].Keys[UninstallString]);
#endif
        nCount++;
      }
    }
    RegCloseKey(hKey);
  }
  p = (KeyInfo *) realloc(p, sizeof(KeyInfo) * nCount);
  FSF.qsort(p, nCount, sizeof(KeyInfo), CompareEntries);

  FLI = (FarListItem *) realloc(FLI, sizeof(FarListItem) * nCount);
  ZeroMemory(FLI, sizeof(FarListItem) * nCount);

  FL.ItemsNumber = nCount;
  FL.Items = FLI;

  TCHAR FirstChar[4];
  FirstChar[0] = _T('&');
  FirstChar[1] = 0;
  FirstChar[2] = _T(' ');
  FirstChar[3] = 0;
  for (int i=0;i<nCount;i++)
  {
#ifdef FARAPI18
    FLI[i].Text = p[i].ListItem;
#endif
    TCHAR* text = const_cast<TCHAR*>(FLI[i].Text);

    if (FirstChar[1] != FSF.LUpper(p[i].Keys[DisplayName][0]))
    {
      FirstChar[1] = FSF.LUpper(p[i].Keys[DisplayName][0]);
      lstrcpy(text, FirstChar);
    }
    else
      lstrcpy(text, EMPTYSTR);

    lstrcat(text, p[i].Keys[DisplayName]);
  }

  ListSize = nCount;
}
#undef JUMPREALLOC
#undef EMPTYSTR

//-------------------------------------------------------------------

struct Options
{
  int WhereWork; //<- TechInfo
  int EnterFunction; //<- TechInfo
} Opt;

void ReadRegistry()
{
  //TechInfo
  if (GetRegKey(HKCU,_T(""),_T("WhereWork"),Opt.WhereWork,3))
    if ((Opt.WhereWork<0) || (Opt.WhereWork>3))
      Opt.WhereWork=3;
  SetRegKey(HKCU,_T(""),_T("WhereWork"),(DWORD) Opt.WhereWork);

  if (GetRegKey(HKCU,_T(""),_T("EnterFunction"),Opt.EnterFunction,0))
    if ((Opt.EnterFunction<0) || (Opt.EnterFunction>1))
      Opt.EnterFunction=0;
  SetRegKey(HKCU,_T(""),_T("EnterFunction"),(DWORD) Opt.EnterFunction);
}
