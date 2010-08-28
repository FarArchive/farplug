@call "%VCINSTALLDIR%\vcvarsall.bat" x86

nmake -nologo RELEASE=1 clean
nmake -nologo RELEASE=1 distrib
@if errorlevel 1 goto error
@copy Release.x86\*.7z .
nmake -nologo RELEASE=1 clean

@call "%VCINSTALLDIR%\vcvarsall.bat" x86_amd64

nmake -nologo RELEASE=1 PLATFORM=x64 clean
nmake -nologo RELEASE=1 PLATFORM=x64 distrib
@if errorlevel 1 goto error
@copy Release.x64\*.7z .
nmake -nologo RELEASE=1 PLATFORM=x64 clean

@goto end

:error
@echo TERMINATED WITH ERRORS

:end
