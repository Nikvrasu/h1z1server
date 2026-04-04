@echo off
title Zone Log Scanner - CH2 RAW Only

IF NOT EXIST "zone_log.txt" (
    echo Error: zone_log.txt not found in this folder.
    pause
    exit /b
)

echo Scanning zone_log.txt for [CH2 RAW]...
echo Saving results to: ch2_raw_only.txt

:: /n - Displays line numbers
:: /c - Search for the literal string "[CH2 RAW]"
findstr /n /c:"[CH2 RAW]" "zone_log.txt" > ch2_raw_only.txt

echo Done. Results written to ch2_raw_only.txt.
pause