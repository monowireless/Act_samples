@echo off

if not exist %MWSDK_ROOT_WINNAME%\000manifest GOTO ENVERR

SET PATH=%MWSDK_ROOT_WINNAME%\..\Tools\MinGW\msys\1.0\bin;%PATH%

SET F=%~n0
SET FARG=%F:build-=%

echo %FARG%
if "%FARG%"=="build" GOTO NONE
if "%FARG%"=="clean" GOTO CLEAN
if "%FARG%"=="cleanall" GOTO CLEAN

:BUILD
make -j4 TWELITE=%FARG% all
pause
exit

:CLEAN
make cleanall
pause
exit

:NONE
echo "nothing performed"
pause
exit

:ENVERR
echo ERROR: The environment variable MWSDK_ROOT MWSDK_ROOT_WINNAME are not set.
pause
exit
