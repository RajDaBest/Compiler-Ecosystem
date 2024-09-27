@echo off
setlocal

rem Set the flags for compilation
set CFLAGS=/W4 /O2 /arch:AVX2 /fp:fast /favor:blend /GL /Zi
set LIBS=

rem Create the output directory if it doesn't exist
if not exist bin (
    mkdir bin
)

rem Build the virtmach executable
cl %CFLAGS% /Fe:bin\virtmach.exe src\main.c %LIBS%

rem Build the devasms executable
cl %CFLAGS% /Fe:bin\devasm.exe src\devasm.c %LIBS%

rem Clean up the build
if "%1"=="clean" (
    del /Q bin\virtmach.exe
    del /Q bin\devasm.exe
)

endlocal
