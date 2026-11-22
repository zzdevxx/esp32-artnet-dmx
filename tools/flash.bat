@echo off
setlocal
echo ============================================================
echo   ZZLIGHT ESP32 1-Click Flasher (v5.2.0 Hardened Master)
echo ============================================================
pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0flash.ps1" %*
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Flashing gagal. Periksa koneksi USB ESP32.
    pause
    exit /b %ERRORLEVEL%
)
endlocal
