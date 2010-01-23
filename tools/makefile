CPPFLAGS = -nologo -EHsc -I.. user32.lib advapi32.lib

build:
  $(CPP) $(CPPFLAGS) gendep.cpp
  $(CPP) $(CPPFLAGS) msgc.cpp
  $(CPP) $(CPPFLAGS) preproc.cpp
  del /q *.obj
