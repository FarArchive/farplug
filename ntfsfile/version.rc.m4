m4_include(`version.m4')m4_dnl
#include <winres.h>
1 VERSIONINFO
FILEVERSION __VER_MAJOR__,__VER_MINOR__,__VER_PATCH__,__WCREV__
PRODUCTVERSION __VER_MAJOR__,__VER_MINOR__,__VER_PATCH__,__WCREV__
FILEOS VOS_NT_WINDOWS32
FILETYPE VFT_DLL
#ifdef DEBUG 
#  if $WCMOD$
FILEFLAGS VS_FF_DEBUG | VS_FF_SPECIALBUILD
#  else
FILEFLAGS VS_FF_DEBUG
#  endif
#endif
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "04090000"
    BEGIN
      VALUE "FileVersion", "__VER_MAJOR__.__VER_MINOR__.__VER_PATCH__.__WCREV__\0"
      VALUE "ProductVersion", "__VER_MAJOR__.__VER_MINOR__.__VER_PATCH__.__WCREV__\0"
      VALUE "FileDescription", "NTFS File Information Plugin for Far Manager\0"
      VALUE "OriginalFilename", "ntfsfile.dll\0"
#ifdef DEBUG 
#  if $WCMOD$
      VALUE "SpecialBuild", "Debug build. Working copy contains modifications.\0"
#  else
      VALUE "SpecialBuild", "Debug build.\0"
#  endif
#endif
      VALUE "LegalCopyright", "None.\0"
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x0409, 0x0000
  END
END
