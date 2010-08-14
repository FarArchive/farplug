!include project.ini

TOOLSDIR = ..\tools

CPPFLAGS = -nologo -Zi -W3 -Gy -GS -GR -EHsc -MP -c
DEFINES = -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_WIN32_WINNT=0x0501 -D__A_IDXSZ_TYPE__=unsigned -D_FAR_USE_FARFINDDATA -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS
LINKFLAGS = -nologo -debug -incremental:no -map -manifest:no -dynamicbase -nxcompat -largeaddressaware -dll
RCFLAGS = -nologo

!if "$(CPU)" == "AMD64" || "$(PLATFORM)" == "x64"
PLATFORM = x64
LIBSUFFIX = 64
RCFLAGS = $(RCFLAGS) -Dx64
!else
PLATFORM = x86
LINKFLAGS = $(LINKFLAGS) -safeseh
LIBSUFFIX = 32
!endif

!ifdef RELEASE
OUTDIR = Release
DEFINES = $(DEFINES) -DNDEBUG
CPPFLAGS = $(CPPFLAGS) -O1 -GL -MT
LINKFLAGS = $(LINKFLAGS) -opt:ref -opt:icf -LTCG
!else
OUTDIR = Debug
DEFINES = $(DEFINES) -DDEBUG
CPPFLAGS = $(CPPFLAGS) -Od -RTC1 -MTd
LINKFLAGS = $(LINKFLAGS) -fixed:no
LIBSUFFIX=$(LIBSUFFIX)d
!endif

!ifdef OLDFAR
FARSDK = farsdk\ansi
DEFFILE = plugin.def
DEFINES = $(DEFINES) -DFARAPI17
FARBRANCH = 1
!else
FARSDK = farsdk\unicode
DEFFILE = plugin_uni.def
DEFINES = $(DEFINES) -DFARAPI18
FARBRANCH = 2
!endif

OUTDIR = $(OUTDIR).$(PLATFORM).$(FARBRANCH)
INCLUDES = -I$(FARSDK) -Irapi -I.. -I$(OUTDIR)
CPPFLAGS = $(CPPFLAGS) -Fo$(OUTDIR)\ -Fd$(OUTDIR)\ $(INCLUDES) $(DEFINES)
RCFLAGS = $(RCFLAGS) $(INCLUDES) $(DEFINES)

!ifdef BUILD
!include $(OUTDIR)\far.ini
!endif

OBJS = $(OUTDIR)\plugin.obj $(OUTDIR)\rapi_proc.obj $(OUTDIR)\util.obj $(OUTDIR)\ui.obj $(OUTDIR)\dlgapi.obj $(OUTDIR)\options.obj $(OUTDIR)\filepath.obj $(OUTDIR)\file_filters.obj $(OUTDIR)\file_info.obj

LIBS = user32.lib advapi32.lib version.lib ole32.lib oleaut32.lib

project: depfile $(OUTDIR)\far.ini
  $(MAKE) -nologo -$(MAKEFLAGS) build_project BUILD=1

distrib: depfile $(OUTDIR)\far.ini
  $(MAKE) -nologo -$(MAKEFLAGS) build_distrib BUILD=1

build_project: $(OUTDIR)\$(MODULE).dll $(OUTDIR)\$(MODULE)_en.lng $(OUTDIR)\$(MODULE)_ru.lng $(OUTDIR)\$(MODULE)_en.hlf $(OUTDIR)\$(MODULE)_ru.hlf

$(OUTDIR)\$(MODULE).dll: $(OUTDIR)\plugin.def $(OBJS) $(OUTDIR)\headers.pch $(OUTDIR)\version.res project.ini
  link $(LINKFLAGS) -def:$(OUTDIR)\plugin.def -out:$@ $(OBJS) $(OUTDIR)\headers.obj $(OUTDIR)\version.res $(LIBS)

$(OBJS): $(OUTDIR)\headers.pch

.cpp{$(OUTDIR)}.obj::
  $(CPP) $(CPPFLAGS) -Yuheaders.hpp -FIheaders.hpp -Fp$(OUTDIR)\headers.pch $<

$(OUTDIR)\headers.pch: headers.cpp headers.hpp
  $(CPP) $(CPPFLAGS) headers.cpp -Ycheaders.hpp -Fp$(OUTDIR)\headers.pch

depfile: $(OUTDIR) $(OUTDIR)\msg.h
  $(TOOLSDIR)\gendep.exe $(INCLUDES) > $(OUTDIR)\dep.mak

$(OUTDIR)\msg.h $(OUTDIR)\en.lng.1 $(OUTDIR)\ru.lng.1: $(OUTDIR) $(OUTDIR)\en.msg $(OUTDIR)\ru.msg
  $(TOOLSDIR)\msgc -in $(OUTDIR)\en.msg $(OUTDIR)\ru.msg -out $(OUTDIR)\msg.h $(OUTDIR)\en.lng.1 $(OUTDIR)\ru.lng.1

$(OUTDIR)\version.res: $(OUTDIR)\version.rc
  $(RC) $(RCFLAGS) -fo$@ $**

PREPROC = $(TOOLSDIR)\preproc $** $@

$(OUTDIR)\version.rc: project.ini version.rc
  $(PREPROC)

$(OUTDIR)\plugin.def: project.ini $(DEFFILE)
  $(PREPROC)

$(OUTDIR)\en.msg: project.ini en.msg
  $(PREPROC)

$(OUTDIR)\ru.msg: project.ini ru.msg
  $(PREPROC)

$(OUTDIR)\en.hlf.1: project.ini en.hlf
  $(PREPROC)

$(OUTDIR)\ru.hlf.1: project.ini ru.hlf
  $(PREPROC)

$(OUTDIR)\file_id.diz.1: project.ini file_id.diz
  $(PREPROC)

$(OUTDIR)\file_id.diz.2: $(OUTDIR)\far.ini $(OUTDIR)\file_id.diz.1
  $(PREPROC)

!ifdef OLDFAR
CONVCP = $(TOOLSDIR)\convcp $** $@ 866
!else
CONVCP = copy $** $@
!endif

$(OUTDIR)\$(MODULE)_en.lng: $(OUTDIR)\en.lng.1
  $(CONVCP)

$(OUTDIR)\$(MODULE)_ru.lng: $(OUTDIR)\ru.lng.1
  $(CONVCP)

$(OUTDIR)\$(MODULE)_en.hlf: $(OUTDIR)\en.hlf.1
  $(CONVCP)

$(OUTDIR)\$(MODULE)_ru.hlf: $(OUTDIR)\ru.hlf.1
  $(CONVCP)

$(OUTDIR)\file_id.diz: $(OUTDIR)\file_id.diz.2
  $(CONVCP)

$(OUTDIR)\history_en.txt: history_en.txt
  $(CONVCP)

$(OUTDIR)\history_ru.txt: history_ru.txt
  $(CONVCP)

$(OUTDIR)\far.ini: $(FARSDK)\plugin.hpp
  $(TOOLSDIR)\farver $** $@

$(OUTDIR):
  if not exist $(OUTDIR) mkdir $(OUTDIR)

!ifdef BUILD
!include $(OUTDIR)\dep.mak
!endif


DISTRIB = $(OUTDIR)\$(MODULE)_$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)
!ifndef OLDFAR
DISTRIB = $(DISTRIB)_uni
!endif
DISTRIB_FILES = .\$(OUTDIR)\$(MODULE).dll .\$(OUTDIR)\$(MODULE).map .\$(OUTDIR)\$(MODULE)_en.lng .\$(OUTDIR)\$(MODULE)_ru.lng .\$(OUTDIR)\$(MODULE)_en.hlf .\$(OUTDIR)\$(MODULE)_ru.hlf .\$(OUTDIR)\file_id.diz .\$(OUTDIR)\history_en.txt .\$(OUTDIR)\history_ru.txt
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
.SUFFIXES: .wxs
