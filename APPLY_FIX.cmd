@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>nul
if not errorlevel 1 (
    py -3 -X utf8 apply_fix.py
    goto finish
)
where python >nul 2>nul
if not errorlevel 1 (
    python -X utf8 apply_fix.py
    goto finish
)
echo Python 3 is required. No files were changed.
:finish
echo.
pause
