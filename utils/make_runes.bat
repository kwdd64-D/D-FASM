@echo off
setlocal

:: --- EDIT THIS ONE LINE for your machine -------------------------------
:: Full path to your python.exe (the one you installed).
set "PYTHON=C:\Users\Kevin\AppData\Local\Python\pythoncore-3.14-64\python.exe"
:: -------------------------------------------------------------------------
::
:: Usage (run from anywhere; double-clicking also works for the first mode):
::   make_runes.bat           generate into utils\python\output and compare. Changes nothing else.
::   make_runes.bat install   same, then copy the packs to assets\ and the layout to config\.
::   make_runes.bat setup     one-time: install Pillow, the picture library the script needs.

set "SCRIPT=%~dp0python\gen_bank0.py"

if not exist "%PYTHON%" (
    echo [make_runes] Python was not found at:
    echo [make_runes]   %PYTHON%
    echo [make_runes] Edit the PYTHON line at the top of this file.
    goto :failure
)

if /i "%~1"=="setup" (
    echo [make_runes] Installing Pillow...
    "%PYTHON%" -m pip install --upgrade pillow
    if errorlevel 1 goto :failure
    echo [make_runes] Setup finished.
    goto :success
)

"%PYTHON%" -c "import PIL" >nul 2>nul
if errorlevel 1 (
    echo [make_runes] Pillow is not installed yet. Run:  make_runes.bat setup
    goto :failure
)

"%PYTHON%" "%SCRIPT%" %*
if errorlevel 1 (
    echo.
    echo [make_runes] The script reported a problem. Read the messages above.
    goto :failure
)

:success
echo.
echo Press any key to close this window...
pause >nul
endlocal
exit /b 0

:failure
echo.
echo Press any key to close this window...
pause >nul
endlocal
exit /b 1
