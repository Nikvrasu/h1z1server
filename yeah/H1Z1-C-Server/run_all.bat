@echo off
pushd login_binaries
IF EXIST packets rmdir /S /Q packets
start loginServer.exe
popd

pushd zone_binaries
IF EXIST packets rmdir /S /Q packets
start zoneServer.exe
popd
