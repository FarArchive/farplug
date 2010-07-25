@call vcvarsall.bat x86

nmake -nologo RELEASE=1 OLDFAR=1 clean
nmake -nologo RELEASE=1 OLDFAR=1 distrib
@if errorlevel 1 goto error
@copy Release.x86.1\*.7z .
nmake -nologo RELEASE=1 OLDFAR=1 clean

nmake -nologo RELEASE=1 clean
nmake -nologo RELEASE=1 distrib installer
@if errorlevel 1 goto error
@copy Release.x86.2\*.7z .
@copy Release.x86.2\*.msi .
nmake -nologo RELEASE=1 clean

@call vcvarsall.bat x86_amd64

nmake -nologo RELEASE=1 OLDFAR=1 clean
nmake -nologo RELEASE=1 OLDFAR=1 distrib
@if errorlevel 1 goto error
@copy Release.x64.1\*.7z .
nmake -nologo RELEASE=1 OLDFAR=1 clean

nmake -nologo RELEASE=1 clean
nmake -nologo RELEASE=1 distrib installer
@if errorlevel 1 goto error
@copy Release.x64.2\*.7z .
@copy Release.x64.2\*.msi .
nmake -nologo RELEASE=1 clean

@goto end

:error
@echo TERMINATED WITH ERRORS

:end
