@echo off
title H1Z1 Server - Build and Run
cd /d "D:\h1z1server\yeah\H1Z1-C-Server"

echo ============================================================
echo  Building Login Server...
echo ============================================================

IF NOT EXIST login_binaries mkdir login_binaries
pushd schema
IF NOT EXIST output mkdir output
gcc -o schema_tool.exe -g -O2 ../src/schema_tool.c -lm
schema_tool.exe kotk_login_udp_11.schm output/kotk_login_udp_11.c
popd
pushd login_binaries
gcc -shared -o loginModule.dll -g -O2 ../src/kotk_login_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm
gcc -o loginServer.exe -g -O2 ../src/win32_login_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm
popd

echo.
echo ============================================================
echo  Building Zone Server...
echo ============================================================

IF NOT EXIST zone_binaries mkdir zone_binaries
pushd schema
gcc -o schema_tool.exe -g -O2 ../src/schema_tool.c -lm
schema_tool.exe client_protocol_1087.schm output/client_protocol_1087.c
popd
pushd zone_binaries
gcc -shared -o zoneModule.dll -g -O0 ../src/kotk_zone_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm
gcc -o zoneServer.exe -g -O2 ../src/win32_zone_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm
popd

echo.
echo ============================================================
echo  Starting Login Server in new window...
echo ============================================================
pushd login_binaries
IF EXIST packets rmdir /S /Q packets
start "Login Server" cmd /k loginServer.exe
popd

echo.
echo ============================================================
echo  Starting Zone Server (output in THIS window)...
echo ============================================================
pushd zone_binaries
IF EXIST packets rmdir /S /Q packets
zoneServer.exe
popd

echo.
echo ============================================================
echo  Zone server exited (crash or normal exit).
echo  Login server window is still open separately.
echo  Press any key to exit.
echo ============================================================
pause
