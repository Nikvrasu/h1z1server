@echo off
title Zone Log Scanner

IF NOT EXIST "zone_logs.txt" (
    echo Error: zone_logs.txt not found in this folder.
    echo Make sure the file is named correctly and is in the same directory as this script.
    pause
    exit /b
)

echo ============================================================
echo  Scanning zone_logs.txt for Client Events...
echo ============================================================
echo.

:: The /n flag adds the line number so you know exactly where it happened
:: The /c flag specifies the exact string to search for
findstr /n /c:"ClientFinishedLoading" /c:"ClientIsReady" "zone_logs.txt"

echo.
echo ============================================================
echo  Scan complete.
echo ============================================================
pause