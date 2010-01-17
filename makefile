.SILENT:

CPPFLAGS = -nologo -Zi -W3 -Gy -GS -GR -EHsc -MP -c \
-DWIN32_LEAN_AND_MEAN -D_WINDOWS -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_FAR_USE_FARFINDDATA -D_WIN32_WINNT=0x0500
LINKFLAGS = -nologo -debug -incremental:no -stack:10000000 -map /manifest:no -dynamicbase -nxcompat -largeaddressaware -dll

!if "$(CPU)" == "AMD64" || "$(PLATFORM)" == "x64"
PLATFORM = x64
CPPFLAGS = $(CPPFLAGS) -DWIN64
!else
PLATFORM = x86
CPPFLAGS = $(CPPFLAGS) -DWIN32
LINKFLAGS = $(LINKFLAGS) -safeseh
!endif

!ifdef RELEASE
OUTDIR = Release
CPPFLAGS = -DNDEBUG -O2 -GL -MT $(CPPFLAGS)
LINKFLAGS = -opt:ref -opt:icf -LTCG $(LINKFLAGS)
!else
OUTDIR = Debug
CPPFLAGS = -DDEBUG -D_DEBUG -Od -RTC1 -MTd $(CPPFLAGS)
LINKFLAGS = -fixed:no $(LINKFLAGS)
!endif
OUTDIR = $(OUTDIR).$(PLATFORM)
INCLUDES = -I7z -Ifar -I$(OUTDIR)
CPPFLAGS = $(CPPFLAGS) -Fo$(OUTDIR)\ -Fd$(OUTDIR)\ -I$(OUTDIR) $(INCLUDES)

OBJS = $(OUTDIR)\archive.obj $(OUTDIR)\farutils.obj $(OUTDIR)\pathutils.obj $(OUTDIR)\plugin.obj $(OUTDIR)\strutils.obj $(OUTDIR)\sysutils.obj $(OUTDIR)\ui.obj

LIBS = advapi32.lib ole32.lib oleaut32.lib

build: depfile
  $(MAKE) -nologo -$(MAKEFLAGS) -f makefile $(OUTDIR)\7z2.dll DEPFILE=$(OUTDIR)\dep.mak

$(OUTDIR)\7z2.dll: $(OUTDIR) plugin.def $(OBJS) $(OUTDIR)\headers.pch
	link @<<
	$(LINKFLAGS) -def:plugin.def -out:$@ $(OBJS) $(OUTDIR)\headers.obj $(LIBS)
<<

$(OBJS): $(OUTDIR) $(OUTDIR)\headers.pch

.cpp{$(OUTDIR)}.obj::
	$(CPP) @<<
	$(CPPFLAGS) -Yuheaders.hpp -FIheaders.hpp -Fp$(OUTDIR)\headers.pch $<
<<

$(OUTDIR)\headers.pch: $(OUTDIR) headers.cpp headers.hpp
	$(CPP) $(CPPFLAGS) headers.cpp -Ycheaders.hpp -Fp$(OUTDIR)\headers.pch

depfile: $(OUTDIR) $(OUTDIR)\msg.h
  tools\gendep.exe $(INCLUDES) > $(OUTDIR)\dep.mak

$(OUTDIR)\msg.h $(OUTDIR)\en.lng $(OUTDIR)\ru.lng: $(OUTDIR) en.msg ru.msg
  tools\msgc -in en.msg ru.msg -out $(OUTDIR)\msg.h $(OUTDIR)\en.lng $(OUTDIR)\ru.lng

$(OUTDIR):
  if not exist $(OUTDIR) mkdir $(OUTDIR)

!ifdef DEPFILE
!include $(DEPFILE)
!endif


prepare:
  if not exist tools mkdir tools
  cl -nologo -EHsc -Fotools\ -Fetools\gendep.exe gendep.cpp $(LIBS)
  cl -nologo -EHsc -D_UNICODE -Fotools\ -Fetools\msgc.exe msgc.cpp $(LIBS)

clean:
  rd /s /q $(OUTDIR)


.PHONY: build depfile prepare clean
