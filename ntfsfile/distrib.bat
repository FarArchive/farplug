@mkdir .build
@cd .build

@call vcvarsall.bat x86

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
nmake -nologo distrib
@if errorlevel 1 goto error
@copy *.7z ..
@rm -r *

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DFARAPI18=1 ..
nmake -nologo distrib
@if errorlevel 1 goto error
@copy *.7z ..
@rm -r *

@call vcvarsall.bat x86_amd64

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
nmake -nologo distrib
@if errorlevel 1 goto error
@copy *.7z ..
@rm -r *

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DFARAPI18=1 ..
nmake -nologo distrib
@if errorlevel 1 goto error
@copy *.7z ..
@rm -r *

@goto end

:error
@echo TERMINATED WITH ERRORS

:end

@cd ..
@rm -r .build
