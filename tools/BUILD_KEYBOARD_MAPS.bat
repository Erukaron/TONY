@echo off
SETLOCAL EnableDelayedExpansion

set oldDir=%cd%

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
mkdir bin\KeyboardMaps >nul
mkdir build\KeyboardMaps >nul
::echo.

echo Creating working copies
copy src\KeyboardMaps\*.* build\KeyboardMaps\ /Y >nul
copy inc\*.* build\KeyboardMaps\ /Y >nul

:: Nasm needs to load the include files from the current directory, so switch to build dir
cd build\KeyboardMaps

:: for loop
for %%f in (*.ASM) do (
    echo Assemble file: %%f
    nasm %%f -f bin -o ..\..\bin\KeyboardMaps\%%~nf.MAP
    if not exist ..\..\bin\KeyboardMaps\%%~nf.MAP goto THROW_ERROR
)

:: Go back to home dir
cd ..\..

:: Adjust filesize for text files
for %%f in (bin\KeyboardMaps\*.MAP) do (
	echo Adjusting file size for file: %%f
	tools\FAT_RootDir_Creator.exe -fill 2048 -fill-byte 0 -i "%%f"
)
echo.

copy bin\KeyboardMaps\*.MAP files >nul
echo.

:: Wait a little bit, so that the delete does not fail
ping localhost -n 2 > nul

if !removeBin!==true (
	rd /S /Q bin
) else (
	rd /S /Q bin\KeyboardMaps
)

if !removeBuild!==true (
	rd /S /Q build
) else (
	rd /S /Q build\KeyboardMaps
)

cd %oldDir%
echo Done

goto eof

:THROW_ERROR
    cd %oldDir%
    echo. >ERROR
    exit /b 1

:eof
