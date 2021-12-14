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
mkdir bin\COM >nul
mkdir bin\COM\SYS_FLAG >nul
mkdir build\COM >nul
mkdir build\COM\SYS_FLAG >nul
::echo.

echo Creating working copies
:: For ASM files
copy src\COM\*.asm build\COM /Y >nul
copy src\COM\SYS_FLAG\*.asm build\COM\SYS_FLAG /Y >nul
copy inc\*.INC build\COM /Y >nul
copy inc\*.INC build\COM\SYS_FLAG /Y >nul
:: For C files
copy tools\smlrc.exe build\COM /Y >nul
copy tools\smlrc.exe build\COM\SYS_FLAG /Y >nul
copy src\COM\*.c build\COM /Y >nul
copy src\COM\SYS_FLAG\*.c build\COM\SYS_FLAG /Y >nul
copy src\COM\*.h build\COM /Y >nul
copy src\COM\SYS_FLAG\*.h build\COM\SYS_FLAG /Y >nul
copy inc\*.h build\COM /Y >nul
copy inc\*.h build\COM\SYS_FLAG /Y >nul
echo.

:: Nasm needs to load the include files from the current directory, so switch to build dir
cd build\COM
echo Programs
:: Compile c files
for %%f in (*.c) do (
    echo Compile file: %%f
    smlrc -seg16 %%f %%~nf.ASM
    if not exist %%~nf.ASM goto THROW_ERROR
)
::echo.

:: Assemble
for %%f in (*.ASM) do (
    echo Assemble file: %%f
    nasm %%f -f bin -o ..\..\bin\COM\%%~nf.COM
    if not exist ..\..\bin\COM\%%~nf.COM goto THROW_ERROR
)
echo.

:: Now the same for system files
cd SYS_FLAG
echo System files
for %%f in (*.c) do (
    echo Compile file: %%f
    smlrc -seg16 %%f %%~nf.ASM
    if not exist %%~nf.ASM goto THROW_ERROR
)
::echo.

for %%f in (*.ASM) do (
    echo Assemble system file: %%f
    nasm %%f -f bin -o ..\..\..\bin\COM\SYS_FLAG\%%~nf.COM
    if not exist ..\..\..\bin\COM\SYS_FLAG\%%~nf.COM goto THROW_ERROR
)
echo.
cd ..

:: Go back to home dir
cd ..\..

copy bin\COM\*.COM files >nul
copy bin\COM\SYS_FLAG\*.COM files\SYS_FLAG >nul
::echo.

:: Wait a little bit, so that the delete does not fail
::ping localhost -n 2 > nul
goto :eof
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
