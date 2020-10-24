@echo off
SETLOCAL EnableDelayedExpansion

set oldDir=%cd%
color 8f

cd tools
if exist ERROR del ERROR

if "%1" == "keyboard" (
	call BUILD_KEYBOARD_MAPS.BAT
) else (
	if "%1" == "clean" ( 
		call CLEAN.BAT
	) else (
		if "%1" == "img" ( 
			call MAKE_IMG.BAT %2
		) else (
			if "%1" == "test" (
				call TEST.BAT %1
			) else (
				if "%1" == "dbg" (
					call TEST.BAT %1
				) else (
					if "%1" == "font" (
						FAT_RootDir_Creator.exe -start-font-editor-3x5
					) else (
						if "%1" == "all" (
							call MAKE_IMG.BAT %2
							REM Add cd iso
						) else (
							:: default to make all
							if "%1" == "" (
								call MAKE_IMG.BAT
								REM Add cd iso
							) else (
								echo Unknown option: "%1"
							)
						)
					)
				)
			)
		)
	)
)

if exist "ERROR" (
	color 4c
	del ERROR
) else (
	color 2f
)

cd %oldDir%
