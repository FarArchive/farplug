@call setenv /x86

nmake -nologo release=1 clean
nmake -nologo release=1 distrib installer
@if errorlevel 1 goto error
@copy Release.x86\*.7z .
@copy Release.x86\*.msi .
nmake -nologo release=1 clean

@call setenv /x64

nmake -nologo release=1 clean
nmake -nologo release=1 distrib installer
@if errorlevel 1 goto error
@copy Release.x64\*.7z .
@copy Release.x64\*.msi .
nmake -nologo release=1 clean

@goto end

:error
@echo TERMINATED WITH ERRORS

:end
