@echo off

IF EXIST GAME_PACKETS rmdir /S /Q GAME_PACKETS
pushd zone_binaries
start zoneServer.exe
popd
