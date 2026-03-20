@echo off

IF EXIST LOGIN_PACKETS rmdir /S /Q LOGIN_PACKETS
pushd login_binaries
start loginServer.exe
popd
