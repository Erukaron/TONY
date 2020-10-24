@echo off
SETLOCAL EnableDelayedExpansion

set oldDir=%cd%

if exist "Bochs-2.6.11\bochs.exe" (
    if "%1" == "test" (
        Bochs-2.6.11\bochs.exe -f bochsrc.bxrc
    ) else (
        if "%1" == "dbg" (
            Bochs-2.6.11\bochsdbg.exe -f bochsrc_debug.bxrc
        ) else (
            echo Testmode "%1" unknown!
            goto THROW_ERROR
        )
    )
) else (
    echo Bochs executable not found!
    goto THROW_ERROR
)

cd %oldDir%
echo Done

goto eof

:THROW_ERROR
    cd %oldDir%
    echo. >ERROR
    exit /b 1

:eof
