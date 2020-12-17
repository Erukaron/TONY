@echo off
SETLOCAL EnableDelayedExpansion

set oldDir=%cd%

set tidy=true
if "%1"=="no-clean" set tidy=false
if "%1"=="no-cleanup" set tidy=false
if "%1"=="no-tidy" set tidy=false
if "%1"=="dirty" set tidy=false

set fillImage=true
if "%1"=="no-fill" set fillImage=false
if "%1"=="no-expand" set fillImage=false
if "%1"=="small" set fillImage=false

echo ======================================================
echo ===           STARTING BUILD PROCESS               ===
echo ======================================================

if exist "CLEAN.BAT" (
	call CLEAN.BAT
) else (
	echo CLEAN.BAT missing!
	goto THROW_ERROR
)

REM go back to main dir
cd ..

if not exist tools\FAT_RootDir_Creator.exe (
	echo FAT_RootDir_Creator.exe missing!
	goto THROW_ERROR
)

echo Creating directories
mkdir bin
mkdir build
mkdir output
mkdir tmpfiles
mkdir tmpfiles\txt
mkdir tmpfiles\bin
echo.

echo Creating working copies
copy src\*.* build\ /Y
copy inc\*.* build\ /Y
echo.

:: Nasm needs to load the include files from the current directory, so switch to build dir
cd build

for %%f in (*.ASM) do (
	echo Assemble file: %%f
	if "%%f"=="KERNEL.ASM" (
		nasm %%f -f bin -o ..\bin\%%~nf.SYS
		if not exist ..\bin\%%~nf.SYS goto THROW_ERROR
	) else (
		nasm %%f -f bin -o ..\bin\%%~nf.BIN
		if not exist ..\bin\%%~nf.BIN goto THROW_ERROR
	)
)
echo.

:: Adjust filesize for binary files
for %%f in (..\bin\*.*) do (
	echo Adjusting file size for binary: %%f
	..\tools\FAT_RootDir_Creator.exe -fill next -fill-byte 0 -i "%%f"
)
echo.

:: Go back to home dir
cd ..

:: Copy text files
echo Create working copies of static files
copy files\*.CFG tmpfiles\txt /Y
copy files\*.TXT tmpfiles\txt /Y
copy README.MD tmpfiles\txt /Y

:: Copy binary files
copy files\*.MAP tmpfiles\bin /Y
copy files\*.BIN tmpfiles\bin /Y
echo.

:: Adjust filesize for text files
for %%f in (tmpfiles\txt\*.*) do (
	echo Adjusting file size for static file: %%f
	tools\FAT_RootDir_Creator.exe -fill next -fill-byte 32 -fill-last-byte 0 -i "%%f"
)

:: Adjust filesize for binary files
for %%f in (tmpfiles\bin\*.*) do (
	echo Adjusting file size for static file: %%f
	tools\FAT_RootDir_Creator.exe -fill next -fill-byte 0 -i "%%f"
)
echo.

echo Moving adjusted static files to bin directory
copy tmpfiles\txt\*.* bin
copy tmpfiles\bin\*.* bin
echo.

:: Add every file to variable and pass var to rootdir creator with -i kernel as first entry
echo Adding file to root directory: bin\KERNEL.SYS
set rootDirArgs=-i "bin\KERNEL.SYS"
for %%f in (bin\*.*) do (
	if not "%%f" == "bin\BOOT.BIN" (
	if not "%%f" == "bin\KERNEL.SYS" (
	if not "%%f" == "bin\ROOTDIR.BIN" (
	if not "%%f" == "bin\FAT.BIN" (
		echo Adding file to root directory: %%f
		set rootDirArgs=!rootDirArgs! -i "%%f"
	))))
)
echo.

echo Building root directory and file allocation table
tools\FAT_RootDir_Creator.exe !rootDirArgs! -oroot build\rootdir.asm -ofat build\fat.asm -s -sysvol "TONY     "

echo Assemble root directory and file allocation table
nasm build\fat.asm -f bin -o bin\FAT.BIN
if not exist bin\FAT.BIN goto THROW_ERROR
nasm build\rootdir.asm -f bin -o bin\ROOTDIR.BIN
if not exist bin\ROOTDIR.BIN goto THROW_ERROR
echo.


:: Add every file to variable and copy each file to image file
echo Adding file to image: bin\BOOT.BIN
echo Adding file to image: bin\FAT.BIN
echo Adding file to image: bin\FAT.BIN
echo Adding file to image: bin\ROOTDIR.BIN
echo Adding file to image: bin\KERNEL.SYS
set copyArgs="bin\BOOT.BIN" + "bin\FAT.BIN" + "bin\FAT.BIN" + "bin\ROOTDIR.BIN" + "bin\KERNEL.SYS"
for %%f in (bin\*.*) do (
	if not "%%f" == "bin\BOOT.BIN" (
	if not "%%f" == "bin\FAT.BIN" (
	if not "%%f" == "bin\ROOTDIR.BIN" (
	if not "%%f" == "bin\KERNEL.SYS" (
		echo Adding file to image: %%f
		set copyArgs=!copyArgs! + "%%f"
	))))
)
echo.

echo Building image
copy /b %copyArgs% "output\TONY.IMG"
echo.

:: Wait a little bit, so that the delete does not fail
ping localhost -n 1 > nul

if !fillImage!==true (
	echo Adjusting file size of image
	tools\FAT_RootDir_Creator.exe -i "output\TONY.IMG" -fill 1474560
	if not exist "output\TONY.IMG" goto THROW_ERROR
	echo.
)

:: Wait a little bit, so that the delete does not fail
ping localhost -n 2 > nul

if !tidy!==true (
	echo Cleaning up
	rd /S /Q build
	rd /S /Q tmpfiles
	rd /S /Q bin
)

cd %oldDir%
echo Done

goto eof

:THROW_ERROR
	::echo ######################################################
	::echo ###        An ERROR occured while building         ###
	::echo ######################################################
	cd %oldDir%
	echo. >ERROR
	exit /b 1

:eof
