@echo off
IF EXIST GAME_PACKETS rmdir /S /Q GAME_PACKETS
pushd zone_binaries
:: /B runs it in the same window, or you can keep it separate
start /wait zoneServer.exe > log.txt 2>&1
popd