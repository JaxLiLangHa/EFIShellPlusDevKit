@echo off

if /I "%1"=="-?" (
    goto help
)

if /I "%1"=="clean" (
    goto clean
)

if /I "%1"=="all" (
    goto clean
)

:buildproject

set WORKSPACE=%CD%
set IASL_PREFIX=%WORKSPACE%\Tools\
set NASM_PREFIX=%WORKSPACE%\Tools\
set PYTHON_HOME=C:\Python27
set EDK_TOOLS_PATH=%WORKSPACE%\BaseTools

call edksetup.bat

set ACTIVE_PLATFORM=EdkShellPkg\EdkShellPkg.dsc
set TARGET=RELEASE
set TARGET_ARCH=X64
set TOOL_CHAIN_TAG=VS2015x86

call build -p %ACTIVE_PLATFORM% -a %TARGET_ARCH% -b %TARGET% -t %TOOL_CHAIN_TAG%
if %errorlevel% NEQ 0 goto end
xcopy /y %WORKSPACE%\Build\EdkShellPkg\%TARGET%_%TOOL_CHAIN_TAG%\%TARGET_ARCH%\Shell.efi .\ShellX64.efi
goto end

:help
echo ===========================================================
echo buildshell.bat [Arg1]:
echo    Arg1: all   -- full build
echo         clean  -- delete Build/*.*
echo ===========================================================
goto:eof

:clean
if exist Build (
        del /s /q /f Build\*.*
        rd /q /s Build\
)

if /I "%1"=="all" (
    goto buildproject
) else (
    goto end
)
goto:eof

:end
REM
goto:eof
