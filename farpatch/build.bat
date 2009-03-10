unicode_far\tools\m4 -P -I unicode_far installer\installer.mak.m4 > installer.mak
call setenv /x86
nmake -f installer.mak
@if errorlevel 1 goto end
call setenv /x64
nmake -f installer.mak
:end
