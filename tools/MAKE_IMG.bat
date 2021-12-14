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
mkdir bin >nul
mkdir build >nul
mkdir output >nul
mkdir tmpfiles >nul
mkdir tmpfiles\txt >nul
mkdir tmpfiles\bin >nul
mkdir tmpfiles_sys_flag >nul
mkdir tmpfiles_sys_flag\txt >nul
mkdir tmpfiles_sys_flag\bin >nul
echo.

echo Creating working copies
:: ASM
copy src\*.* build\ /Y >nul
copy inc\*.* build\ /Y >nul
:: C
copy tools\smlrc.exe build\ /Y >nul
::echo.

:: Nasm needs to load the include files from the current directory, so switch to build dir
cd build

:: Compile additional (C) files first
for %%f in (*.c) do (
    echo Compile file: %%f
    smlrc -seg16 %%f %%~nf.ASM
    if not exist %%~nf.ASM goto THROW_ERROR
)

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

:: Go back to home dir
cd ..

:: Add every file to variable and copy each file to image file
echo Adding file to image: bin\BOOT.BIN
echo Adding file to image: bin\FAT.BIN
echo Adding file to image: bin\FAT.BIN
echo Adding file to image: bin\ROOTDIR.BIN
echo Adding file to image: bin\KERNEL.SYS
set copyArgs="bin\BOOT.BIN" + "bin\FAT.BIN" + "bin\FAT.BIN" + "bin\ROOTDIR.BIN" + "bin\KERNEL.SYS"
echo.

:: Add every file to variable and pass var to rootdir creator with -i kernel as first entry
echo Adding file to root directory: bin\KERNEL.SYS
set rootDirArgs=-isys "bin\KERNEL.SYS"
for %%f in (bin\*.*) do (
	if not "%%f" == "bin\BOOT.BIN" (
	if not "%%f" == "bin\KERNEL.SYS" (
		echo Adding file to root directory: %%f
		set rootDirArgs=!rootDirArgs! -i "%%f"
	))
)
echo.

:: Add files to image copy string
for %%f in (bin\*.*) do (
	if not "%%f" == "bin\BOOT.BIN" (
	if not "%%f" == "bin\KERNEL.SYS" (
		echo Adding file to image: %%f
		set copyArgs=!copyArgs! + "%%f"
	))
)
echo.

:: Copy text files
echo Create working copies of static files
copy files\*.CFG tmpfiles\txt /Y >nul
copy files\*.TXT tmpfiles\txt /Y >nul
copy files\*.BAT tmpfiles\txt /Y >nul
copy README.MD tmpfiles\txt /Y >nul

:: Copy binary files
copy files\*.MAP tmpfiles\bin /Y >nul
copy files\*.BIN tmpfiles\bin /Y >nul
copy files\*.COM tmpfiles\bin /Y >nul
echo.

:: Copy system files (text)
copy files\SYS_FLAG\*.CFG tmpfiles_sys_flag\txt /Y >nul
::copy files\SYS_FLAG\*.TXT tmpfiles_sys_flag\txt /Y
::copy files\SYS_FLAG\*.BAT tmpfiles_sys_flag\txt /Y

:: Copy system files (bin)
copy files\SYS_FLAG\*.MAP tmpfiles_sys_flag\bin /Y >nul
copy files\SYS_FLAG\*.BIN tmpfiles_sys_flag\bin /Y >nul
copy files\SYS_FLAG\*.COM tmpfiles_sys_flag\bin /Y >nul
echo.

:: Add txt files to root dir
for %%f in (tmpfiles\txt\*.*) do (
	echo Adding file to root directory: %%f
	set rootDirArgs=!rootDirArgs! -i "%%f"
)
echo.

for %%f in (tmpfiles_sys_flag\txt\*.*) do (
	echo Adding system file to root directory: %%f
	set rootDirArgs=!rootDirArgs! -isys "%%f"
)
echo.

:: Add files to image copy string
for %%f in (tmpfiles\txt\*.*) do (
	echo Adding file to image: bin\%%~nxf
	set copyArgs=!copyArgs! + "bin\%%~nxf"
)
echo.

for %%f in (tmpfiles_sys_flag\txt\*.*) do (
	echo Adding system file to image: bin\%%~nxf
	set copyArgs=!copyArgs! + "bin\%%~nxf"
)
echo.

:: Add bin files to root dir
for %%f in (tmpfiles\bin\*.*) do (
	echo Adding file to root directory: %%f
	set rootDirArgs=!rootDirArgs! -i "%%f"
)
echo.

for %%f in (tmpfiles_sys_flag\bin\*.*) do (
	echo Adding system file to root directory: %%f
	set rootDirArgs=!rootDirArgs! -isys "%%f"
)
echo.

:: Add files to image copy string
for %%f in (tmpfiles\bin\*.*) do (
	echo Adding file to image: bin\%%~nxf
	set copyArgs=!copyArgs! + "bin\%%~nxf"
)
echo.

for %%f in (tmpfiles_sys_flag\bin\*.*) do (
	echo Adding system file to image: bin\%%~nxf
	set copyArgs=!copyArgs! + "bin\%%~nxf"
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

:: Adjust file sizes AFTER the root dir was built
:: Adjust filesize for binary files -> They need to be terminated at a multiple of 512 to be loaded correctly
echo Adjusting file sizes for binaries...
for %%f in (bin\*.*) do (
	echo Adjusting file size for binary: %%f >nul
	tools\FAT_RootDir_Creator.exe -fill next -fill-byte 0 -i "%%f" >nul
)
::echo.

:: Adjust filesize for text files -> They need to be terminated at a multiple of 512 to be loaded correctly
echo Adjusting file sizes for static files...
for %%f in (tmpfiles\txt\*.*) do (
	echo Adjusting file size for static file: %%f >nul
	tools\FAT_RootDir_Creator.exe -fill next -fill-byte 32 -fill-last-byte 0 -i "%%f" >nul
)
echo Adjusting file sizes for static system files...
for %%f in (tmpfiles_sys_flag\txt\*.*) do (
	echo Adjusting file size for static system file: %%f >nul
	tools\FAT_RootDir_Creator.exe -fill next -fill-byte 32 -fill-last-byte 0 -i "%%f" >nul
)

:: Adjust filesize for binary files -> They need to be terminated at a multiple of 512 to be loaded correctly
echo Adjusting file sizes for binaries...
for %%f in (tmpfiles\bin\*.*) do (
	echo Adjusting file size for static file: %%f >nul
	tools\FAT_RootDir_Creator.exe -fill next -fill-byte 0 -i "%%f" >nul
)
echo Adjusting file sizes for system binaries...
for %%f in (tmpfiles_sys_flag\bin\*.*) do (
	echo Adjusting file size for static system file: %%f >nul
	tools\FAT_RootDir_Creator.exe -fill next -fill-byte 0 -i "%%f" >nul
)
echo.

echo Moving adjusted static files to bin directory
copy tmpfiles\txt\*.* bin >nul
copy tmpfiles\bin\*.* bin >nul
copy tmpfiles_sys_flag\txt\*.* bin >nul
copy tmpfiles_sys_flag\bin\*.* bin >nul
echo.

echo Building image
copy /b !copyArgs! "output\TONY.IMG" >nul
::echo.

:: Wait a little bit, so that the adjustment works
::ping localhost -n 1 > nul

if !fillImage!==true (
	echo Adjusting file size of image
	tools\FAT_RootDir_Creator.exe -i "output\TONY.IMG" -fill 1474560 >nul
	if not exist "output\TONY.IMG" goto THROW_ERROR
	echo.
)

echo Done

if !tidy!==true (
	:: Wait a little bit, so that the delete does not fail
	ping localhost -n 3 > nul

	echo Cleaning up
	rd /S /Q build
	rd /S /Q tmpfiles
	rd /S /Q tmpfiles_sys_flag
	rd /S /Q bin
)

cd %oldDir%

goto eof

:THROW_ERROR
	::echo ######################################################
	::echo ###        An ERROR occured while building         ###
	::echo ######################################################
	cd %oldDir%
	echo. >ERROR
	exit /b 1

:eof
