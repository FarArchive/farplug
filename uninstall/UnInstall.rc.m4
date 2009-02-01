m4_include(`version.m4')m4_dnl
#include <winresrc.h>
1 VERSIONINFO
FILEVERSION __VER_MAJOR__,__VER_MINOR__,__VER_PATCH__,__WCREV__
PRODUCTVERSION __VER_MAJOR__,__VER_MINOR__,__VER_PATCH__,__WCREV__
FILEOS VOS__WINDOWS32
FILETYPE VFT_DLL
#ifdef DEBUG
# if __WCMOD__
FILEFLAGS VS_FF_DEBUG | VS_FF_SPECIALBUILD
# else
FILEFLAGS VS_FF_DEBUG
# endif
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
   VALUE "FileVersion", "__VER_MAJOR__.__VER_MINOR__.__VER_PATCH__.__WCREV__\0"
   VALUE "OriginalFilename", "UnInstall.dll\0"
#ifdef DEBUG 
# if __WCMOD__
   VALUE "SpecialBuild", "Debug build. Working copy contains modifications.\0"
# else
   VALUE "SpecialBuild", "Debug build.\0"
# endif
#endif
  }
 }
 BLOCK "VarFileInfo"
 {
  VALUE "Translation", 0x0409, 0x0000
 }
}
