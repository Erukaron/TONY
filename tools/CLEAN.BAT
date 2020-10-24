@echo off
SETLOCAL EnableDelayedExpansion

REM go back to main dir
cd ..

echo Cleaning up
rd /S /Q build
rd /S /Q tmpfiles
rd /S /Q bin
rd /S /Q output
echo.
