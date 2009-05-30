all:
  cd unicode_far
  nmake -f makefile_vc MP=/MP
  cd ..\misc\fexcept
  nmake -f makefile_vc WIDE=1 MP=/MP
  cd ..\..\plugins
  nmake -f makefile_all_vc WIDE=1 MP=/MP
  cd multiarc
  nmake -f makefile_vc MP=/MP
  cd ..\ftp
  nmake -f makefile_vc MP=/MP
  cd ..\..
  unicode_far\tools\m4 -P -I unicode_far installer\installer.mak.m4 > installer.mak
