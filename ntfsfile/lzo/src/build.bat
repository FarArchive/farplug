call %VS80COMNTOOLS%\..\..\VC\vcvarsall.bat
call B\win32\vc.bat debug
call B\win32\vc.bat
call %VS80COMNTOOLS%\..\..\VC\vcvarsall.bat x86_amd64
call B\win64\vc.bat debug
call B\win64\vc.bat
