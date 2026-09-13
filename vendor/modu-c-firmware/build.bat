@echo off
setlocal

set "ZMK_APP=%~1"
if not defined ZMK_APP set "ZMK_APP=C:\zmk\app"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -ZmkApp "%ZMK_APP%"
exit /b %ERRORLEVEL%
