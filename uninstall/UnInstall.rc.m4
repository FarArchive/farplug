m4_changecom()m4_dnl
#include <winres.h>
1 VERSIONINFO
FILEVERSION __VER_MAJOR__, __VER_MINOR__, __VER_PATCH__, 0
PRODUCTVERSION __VER_MAJOR__, __VER_MINOR__, __VER_PATCH__, 0
FILEOS VOS__WINDOWS32
FILETYPE VFT_DLL
#ifdef DEBUG 
FILEFLAGS VS_FF_DEBUG
#endif
{
 BLOCK "StringFileInfo"
 {
  BLOCK "04090000"
  {
   VALUE "CompanyName", "Eugene Sementsov\0"
   VALUE "FileDescription", "UnInstall PlugIn Module for Far Manager\0"
   VALUE "Comments", "Find> [rels@inbox.ru | 2:5010/272.1]\0"
   VALUE "LegalCopyright", "\251 Eugene Sementsov\0"
   VALUE "FileVersion", "__VER_MAJOR__.__VER_MINOR__.__VER_PATCH__.0\0"
   VALUE "OriginalFilename", "UnInstall.dll\0"
#ifdef DEBUG 
   VALUE "SpecialBuild", "Debug build.\0"
#endif
  }
 }
 BLOCK "VarFileInfo"
 {
  VALUE "Translation", 0x0409, 0x0000
 }
}
