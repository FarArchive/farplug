!include project.ini

COMMONDIR = ..\common
TOOLSDIR = ..\tools

CPPFLAGS = -nologo -Zi -W3 -Gy -GS -GR -EHsc -MP -c
DEFINES = -DWIN32_LEAN_AND_MEAN -D_WINDOWS -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_WIN32_WINNT=0x0500
LINKFLAGS = -nologo -debug -incremental:no -map -manifest:no -dynamicbase -nxcompat -largeaddressaware -dll
RCFLAGS = -nologo

!if "$(CPU)" == "AMD64" || "$(PLATFORM)" == "x64"
PLATFORM = x64
!else
PLATFORM = x86
LINKFLAGS = $(LINKFLAGS) -safeseh
!endif
DEFINES = $(DEFINES) -DPLATFORM=$(PLATFORM)

!ifdef RELEASE
OUTDIR = Release
DEFINES = $(DEFINES) -DNDEBUG
CPPFLAGS = $(CPPFLAGS) -O2 -GL -MT
LINKFLAGS = $(LINKFLAGS) -opt:ref -opt:icf -LTCG
!else
OUTDIR = Debug
DEFINES = $(DEFINES) -DDEBUG
CPPFLAGS = $(CPPFLAGS) -Od -RTC1 -MTd
LINKFLAGS = $(LINKFLAGS) -fixed:no
!endif
OUTDIR = $(OUTDIR).$(PLATFORM)
INCLUDES = -I7z -Ifar -I$(OUTDIR) -I$(COMMONDIR)
CPPFLAGS = $(CPPFLAGS) -Fo$(OUTDIR)\ -Fd$(OUTDIR)\ $(INCLUDES) $(DEFINES)
RCFLAGS = $(RCFLAGS) $(INCLUDES) $(DEFINES)

OBJS = $(OUTDIR)\archive.obj $(OUTDIR)\farutils.obj $(OUTDIR)\pathutils.obj $(OUTDIR)\plugin.obj $(OUTDIR)\strutils.obj $(OUTDIR)\sysutils.obj $(OUTDIR)\ui.obj $(OUTDIR)\extract.obj

LIBS = user32.lib advapi32.lib ole32.lib oleaut32.lib

project: depfile
  $(MAKE) -nologo -$(MAKEFLAGS) build_project BUILD=1

distrib: depfile
  $(MAKE) -nologo -$(MAKEFLAGS) build_distrib BUILD=1

build_project: $(OUTDIR)\$(MODULE).dll $(OUTDIR)\en.lng $(OUTDIR)\ru.lng $(OUTDIR)\en.hlf $(OUTDIR)\ru.hlf

$(OUTDIR)\$(MODULE).dll: $(OUTDIR)\plugin.def $(OBJS) $(OUTDIR)\headers.pch $(OUTDIR)\version.res project.ini
  link $(LINKFLAGS) -def:$(OUTDIR)\plugin.def -out:$@ $(OBJS) $(OUTDIR)\headers.obj $(OUTDIR)\version.res $(LIBS)

$(OBJS): $(OUTDIR)\headers.pch

.cpp{$(OUTDIR)}.obj::
  $(CPP) $(CPPFLAGS) -Yuheaders.hpp -FIheaders.hpp -Fp$(OUTDIR)\headers.pch $<

{$(COMMONDIR)}.cpp{$(OUTDIR)}.obj::
  $(CPP) $(CPPFLAGS) -Yuheaders.hpp -FIheaders.hpp -Fp$(OUTDIR)\headers.pch $<

$(OUTDIR)\headers.pch: headers.cpp headers.hpp
  $(CPP) $(CPPFLAGS) headers.cpp -Ycheaders.hpp -Fp$(OUTDIR)\headers.pch

depfile: $(OUTDIR) $(OUTDIR)\msg.h
  $(TOOLSDIR)\gendep.exe $(INCLUDES) $(COMMONDIR) > $(OUTDIR)\dep.mak

$(OUTDIR)\msg.h $(OUTDIR)\en.lng $(OUTDIR)\ru.lng: $(OUTDIR) $(OUTDIR)\en.msg $(OUTDIR)\ru.msg
  $(TOOLSDIR)\msgc -in $(OUTDIR)\en.msg $(OUTDIR)\ru.msg -out $(OUTDIR)\msg.h $(OUTDIR)\en.lng $(OUTDIR)\ru.lng

$(OUTDIR)\version.res: $(OUTDIR)\version.rc
  $(RC) $(RCFLAGS) -fo$@ $**

PREPROC = $(TOOLSDIR)\preproc $** $@

$(OUTDIR)\version.rc: project.ini version.rc
  $(PREPROC)

$(OUTDIR)\plugin.def: project.ini plugin.def
  $(PREPROC)

$(OUTDIR)\en.msg: project.ini en.msg
  $(PREPROC)

$(OUTDIR)\ru.msg: project.ini ru.msg
  $(PREPROC)

$(OUTDIR)\en.hlf: project.ini en.hlf
  $(PREPROC)

$(OUTDIR)\ru.hlf: project.ini ru.hlf
  $(PREPROC)

$(OUTDIR):
  if not exist $(OUTDIR) mkdir $(OUTDIR)

!ifdef BUILD
!include $(OUTDIR)\dep.mak
!endif


DISTRIB = $(OUTDIR)\$(MODULE)_$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)_uni
DISTRIB_FILES = .\$(OUTDIR)\$(MODULE).dll .\$(OUTDIR)\$(MODULE).map .\$(OUTDIR)\en.lng .\$(OUTDIR)\ru.lng .\$(OUTDIR)\en.hlf .\$(OUTDIR)\ru.hlf
!if "$(PLATFORM)" != "x86"
DISTRIB = $(DISTRIB)_$(PLATFORM)
!endif
!ifndef RELEASE
DISTRIB = $(DISTRIB)_dbg
DISTRIB_FILES = $(DISTRIB_FILES) .\$(OUTDIR)\$(MODULE).pdb
!endif

build_distrib: $(DISTRIB).7z

$(DISTRIB).7z: $(DISTRIB_FILES) project.ini
  7z a -mx=9 $@ $(DISTRIB_FILES)


clean:
  if exist $(OUTDIR) rd /s /q $(OUTDIR)


.PHONY: project distrib build_project build_distrib depfile clean
