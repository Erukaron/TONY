@echo off
SETLOCAL EnableDelayedExpansion

set oldDir=%cd%

echo ======================================================
echo ===                  BUILDING COMS                 ===
echo ======================================================

REM go back to main dir
cd ..

if not exist tools\FAT_RootDir_Creator.exe (
    echo FAT_RootDir_Creator.exe missing!
    goto THROW_ERROR
)

set removeBin=true
set removeBuild=true
if exist bin\ set removeBin=false
if exist build\ set removeBuild=false

echo Creating directories
mkdir bin\COM
mkdir bin\COM\SYS_FLAG
mkdir build\COM
mkdir build\COM\SYS_FLAG
echo.

echo Creating working copies
copy src\COM\*.* build\COM /Y
copy src\COM\SYS_FLAG\*.* build\COM\SYS_FLAG /Y
copy inc\*.* build\COM /Y
copy inc\*.* build\COM\SYS_FLAG /Y
echo.

:: Nasm needs to load the include files from the current directory, so switch to build dir
cd build\COM

for %%f in (*.ASM) do (
    echo Assemble file: %%f
    nasm %%f -f bin -o ..\..\bin\COM\%%~nf.COM
    if not exist ..\..\bin\COM\%%~nf.COM goto THROW_ERROR
)
echo.

cd SYS_FLAG
for %%f in (*.ASM) do (
    echo Assemble system file: %%f
    nasm %%f -f bin -o ..\..\..\bin\COM\SYS_FLAG\%%~nf.COM
    if not exist ..\..\..\bin\COM\SYS_FLAG\%%~nf.COM goto THROW_ERROR
)
echo.
cd ..

:: Go back to home dir
cd ..\..

copy bin\COM\*.COM files
copy bin\COM\SYS_FLAG\*.COM files\SYS_FLAG
echo.

:: Wait a little bit, so that the delete does not fail
::ping localhost -n 2 > nul

if !removeBin!==true (
    rd /S /Q bin
) else (
    rd /S /Q bin\COM
)

if !removeBuild!==true (
    rd /S /Q build
) else (
    rd /S /Q build\COM
)

cd %oldDir%
echo Done

goto eof

:THROW_ERROR
    cd %oldDir%
    echo. >ERROR
    exit /b 1

:eof
