@echo off
rem Double-click to make a plain copy of Gordon's Reloading Tool's database (see grt_decrypt_db.ps1).
rem This file and grt_decrypt_db.ps1 must be in the folder of GRT (next to GordonsReloadingTool.exe).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0grt_decrypt_db.ps1" %*
echo.
pause
