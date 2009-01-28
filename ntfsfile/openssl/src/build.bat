call %VS80COMNTOOLS%\..\..\VC\vcvarsall.bat
perl Configure VC-WIN32
call ms\do_ms debug
nmake -f ms\nt.mak
call ms\do_masm
nmake -f ms\nt.mak
call %VS80COMNTOOLS%\..\..\VC\vcvarsall.bat x86_amd64
perl Configure VC-WIN64A
call ms\do_win64a debug
nmake -f ms\nt.mak
call ms\do_win64a
nmake -f ms\nt.mak
