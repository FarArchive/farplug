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

const int KeysCount = ARRAYSIZE(HelpTopics);
struct RegKeyPath {
  HKEY Root;
  const TCHAR* Path;
} UninstKeys[] = {
  { HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
  { HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall") },
};
int nCount, nRealCount;
FarList FL;
FarListItem* FLI = NULL;
int ListSize;
HANDLE hStdout;

struct KeyInfo
{
  TCHAR Keys[KeysCount][MAX_PATH];
#ifdef FARAPI18
  TCHAR ListItem[MAX_PATH];
#endif
  bool Avail[KeysCount];
  RegKeyPath RegKey;
  REGSAM RegView;
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
bool FillReg(KeyInfo& key, TCHAR* Buf, RegKeyPath& RegKey, REGSAM RegView)
{
  HKEY userKey;
  DWORD regType;
  TCHAR fullN[MAX_PATH];
  LONG ExitCode;
  DWORD bufSize;

  key.RegKey = RegKey;
  key.RegView = RegView;
  StringCchCopy(key.SubKeyName,ARRAYSIZE(key.SubKeyName),Buf);
  StringCchCopy(fullN,ARRAYSIZE(fullN),key.RegKey.Path);
  StringCchCat(fullN,ARRAYSIZE(fullN),_T("\\"));
  StringCchCat(fullN,ARRAYSIZE(fullN),key.SubKeyName);
  if (RegOpenKeyEx(key.RegKey.Root, fullN, 0, KEY_READ | RegView, &userKey) != ERROR_SUCCESS)
    return FALSE;
  key.WindowsInstaller = (RegQueryValueEx(userKey,_T("WindowsInstaller"),0,NULL,NULL,NULL) == ERROR_SUCCESS) && ValidGuid(key.SubKeyName);
  for (int i=0;i<KeysCount;i++)
  {
    bufSize = MAX_PATH * sizeof(TCHAR);
    ExitCode = RegQueryValueEx(userKey,HelpTopics[i],0,&regType,(LPBYTE)key.Keys[i],&bufSize);
    key.Keys[i][ARRAYSIZE(key.Keys[i]) - 1] = 0;
    if (ExitCode != ERROR_SUCCESS || lstrcmp(key.Keys[i],_T("")) == 0)
    {
      if ((i == UninstallString && !key.WindowsInstaller) || i == DisplayName)
      {
        RegCloseKey(userKey);
        return FALSE;
      }
      StringCchCopy(key.Keys[i],ARRAYSIZE(key.Keys[i]),_T(""));
      key.Avail[i] = FALSE;
    }
    else
      key.Avail[i] = TRUE;
  }
  RegCloseKey(userKey);
  return TRUE;
}

#ifdef FARAPI17
#define DM_GETDLGITEMSHORT DM_GETDLGITEM
#endif

LONG_PTR WINAPI EntryDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
  switch(Msg)
  {
    case DN_INITDIALOG:
    {
      FarDialogItem item;
      for (unsigned id = 0; Info.SendDlgMessage(hDlg, DM_GETDLGITEMSHORT, id, reinterpret_cast<LONG_PTR>(&item)); id++)
      {
        if (item.Type == DI_EDIT)
          Info.SendDlgMessage(hDlg, DM_EDITUNCHANGEDFLAG, id, 0);
      }
    }
    break;
  }
  return Info.DefDlgProc(hDlg,Msg,Param1,Param2);
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
  Info.DialogEx(Info.ModuleNumber, -1, -1, sx + 8, sy + 4, NULL, DialogItems, di_cnt, 0, 0, EntryDlgProc, 0);
#endif
#ifdef FARAPI18
  HANDLE h_dlg = Info.DialogInit(Info.ModuleNumber, -1, -1, sx + 8, sy + 4, NULL, DialogItems, di_cnt, 0, 0, EntryDlgProc, 0);
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
    StringCchCopy(cmd_line, ARRAYSIZE(cmd_line), _T("msiexec /x"));
    StringCchCat(cmd_line, ARRAYSIZE(cmd_line), p[Sel].SubKeyName);
  }
  else StringCchCopy(cmd_line, ARRAYSIZE(cmd_line), p[Sel].Keys[UninstallString]);

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
  GetConsoleTitle(SaveTitle,ARRAYSIZE(SaveTitle));
  SaveTitle[ARRAYSIZE(SaveTitle) - 1] = 0;
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

typedef WINADVAPI LSTATUS (APIENTRY *FRegDeleteKeyExA)(__in HKEY hKey, __in LPCSTR lpSubKey, __in REGSAM samDesired, __reserved DWORD Reserved);
typedef WINADVAPI LSTATUS (APIENTRY *FRegDeleteKeyExW)(__in HKEY hKey, __in LPCWSTR lpSubKey, __in REGSAM samDesired, __reserved DWORD Reserved);

bool DeleteEntry(int Sel)
{
  HMODULE h_mod = LoadLibrary(_T("advapi32.dll"));
  FRegDeleteKeyExA RegDeleteKeyExA = reinterpret_cast<FRegDeleteKeyExA>(GetProcAddress(h_mod, "RegDeleteKeyExA"));
  FRegDeleteKeyExW RegDeleteKeyExW = reinterpret_cast<FRegDeleteKeyExW>(GetProcAddress(h_mod, "RegDeleteKeyExW"));
  FreeLibrary(h_mod);

  HKEY userKey;
  LONG ret = RegOpenKeyEx(p[Sel].RegKey.Root, p[Sel].RegKey.Path, 0, DELETE | p[Sel].RegView, &userKey);
  if (ret != ERROR_SUCCESS) return false;
  if (RegDeleteKeyEx)
    ret = RegDeleteKeyEx(userKey, p[Sel].SubKeyName, p[Sel].RegView, 0);
  else
    ret = RegDeleteKey(userKey, p[Sel].SubKeyName);
  RegCloseKey(userKey);
  if (ret != ERROR_SUCCESS) return false;
  return true;
}

//сравнить строки
int __cdecl CompareEntries(const void* item1, const void* item2)
{
  return FSF.LStricmp(reinterpret_cast<const KeyInfo*>(item1)->Keys[DisplayName], reinterpret_cast<const KeyInfo*>(item2)->Keys[DisplayName]);
}

#define JUMPREALLOC 50
void EnumKeys(RegKeyPath& RegKey, REGSAM RegView = 0) {
  HKEY hKey;
  if (RegOpenKeyEx(RegKey.Root, RegKey.Path, 0, KEY_READ | RegView, &hKey) != ERROR_SUCCESS)
    return;
  DWORD cSubKeys;
  if (RegQueryInfoKey(hKey,NULL,NULL,NULL,&cSubKeys,NULL,NULL,NULL,NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
    return;

  TCHAR Buf[MAX_PATH];
  for (DWORD fEnumIndex=0; fEnumIndex<cSubKeys; fEnumIndex++)
  {
    DWORD bufSize = ARRAYSIZE(Buf);
    FILETIME ftLastWrite;
    if (RegEnumKeyEx(hKey, fEnumIndex, Buf, &bufSize, NULL, NULL, NULL, &ftLastWrite) != ERROR_SUCCESS)
      return;
    if (nCount >= nRealCount)
    {
      nRealCount += JUMPREALLOC;
      p = (KeyInfo *) realloc(p, sizeof(KeyInfo) * nRealCount);
    }
    if (FillReg(p[nCount], Buf, RegKey, RegView))
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
#undef JUMPREALLOC

typedef WINBASEAPI VOID (WINAPI *FGetNativeSystemInfo)(__out LPSYSTEM_INFO lpSystemInfo);

#define EMPTYSTR _T("  ")
//Обновление информации
void UpDateInfo(void)
{
  HMODULE h_mod = LoadLibrary(_T("kernel32.dll"));
  FGetNativeSystemInfo GetNativeSystemInfo = reinterpret_cast<FGetNativeSystemInfo>(GetProcAddress(h_mod, "GetNativeSystemInfo"));
  FreeLibrary(h_mod);
  bool is_os_x64 = false;
  if (GetNativeSystemInfo)
  {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    is_os_x64 = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
  }

  nCount = nRealCount = 0;
  for (int i=0;i<ARRAYSIZE(UninstKeys);i++)
  {
    if (is_os_x64)
    {
      EnumKeys(UninstKeys[i], KEY_WOW64_64KEY);
      EnumKeys(UninstKeys[i], KEY_WOW64_32KEY);
    }
    else
    {
      EnumKeys(UninstKeys[i]);
    }
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
#ifdef FARAPI17
    size_t MaxSize = ARRAYSIZE(FLI[i].Text);
#endif
#ifdef FARAPI18
    size_t MaxSize = ARRAYSIZE(p[i].ListItem);
    FLI[i].Text = p[i].ListItem;
#endif
    TCHAR* text = const_cast<TCHAR*>(FLI[i].Text);

    if (FirstChar[1] != FSF.LUpper(p[i].Keys[DisplayName][0]))
    {
      FirstChar[1] = FSF.LUpper(p[i].Keys[DisplayName][0]);
      StringCchCopy(text, MaxSize, FirstChar);
    }
    else
      StringCchCopy(text, MaxSize, EMPTYSTR);

    StringCchCat(text, MaxSize, p[i].Keys[DisplayName]);
  }

  ListSize = nCount;
}
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
