@echo off
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

@echo on
@ call build -p %ACTIVE_PLATFORM% -a %TARGET_ARCH% -b %TARGET% -t %TOOL_CHAIN_TAG%
xcopy /y %WORKSPACE%\Build\EdkShellPkg\%TARGET%_%TOOL_CHAIN_TAG%\%TARGET_ARCH%\ShellFull.efi .\ShellX64.efi