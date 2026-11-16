@echo off
setlocal

:: --- EDIT THIS ONE LINE for your machine -------------------------------
:: Point it at the INCLUDE folder inside your own FASM install.
:: This is the only machine-specific setting in the whole project -
:: it never gets baked into d_fasm.asm.
set FASM_INCLUDE=C:\fasm\INCLUDE
:: -------------------------------------------------------------------------

set INCLUDE=%FASM_INCLUDE%
set "OUTPUT=%~dp0build\d_fasm.exe"

where fasm >nul 2>nul
if errorlevel 1 (
    echo [build.bat] "fasm" was not found on PATH.
    echo [build.bat] Add your FASM folder to PATH, or edit this script to call it by full path.
    goto :failure
)

if not exist "%~dp0build" mkdir "%~dp0build"

fasm "%~dp0src\d_fasm.asm" "%OUTPUT%"
if errorlevel 1 (
    echo [build.bat] Build failed.
    goto :failure
)

if not exist "%OUTPUT%" (
    echo [build.bat] FASM reported success, but the output file was not found.
    goto :failure
)

echo.
echo [build.bat] Build succeeded.
for %%F in ("%OUTPUT%") do (
    echo [build.bat] Output: %%~fF
    echo [build.bat] Size: %%~zF bytes
    echo [build.bat] Modified: %%~tF
)
echo.
echo Press any key to close this window...
pause >nul
endlocal
exit /b 0

:failure
echo.
if exist "%OUTPUT%" echo [build.bat] An output file exists, but this run did not confirm it is current.
echo Press any key to close this window...
pause >nul
endlocal
exit /b 1
