@call setenv /x86

nmake -nologo release=1 distrib
@if errorlevel 1 goto error
@copy Release.x86\*.7z .
nmake -nologo release=1 clean

@call setenv /x64

nmake -nologo release=1 distrib
@if errorlevel 1 goto error
@copy Release.x64\*.7z .
nmake -nologo release=1 clean

@goto end

:error
@echo TERMINATED WITH ERRORS

:end
