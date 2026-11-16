@echo off
set ROOT=D:\projects\D\d_fasm
set OUT=D:\projects\D\d_fasm\docs\d_fasm_filelist.md
chcp 65001 > nul

echo # D File List > "%OUT%"
echo. >> "%OUT%"
echo Generated: %date% %time% >> "%OUT%"
echo. >> "%OUT%"
echo ``` >> "%OUT%"

tree "%ROOT%" /F /A | findstr /V "\.import" >> "%OUT%"

echo ``` >> "%OUT%"
echo Done! d-filelist.md.
pause