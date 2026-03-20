@echo off

IF NOT EXIST login_binaries mkdir login_binaries
pushd login_binaries

cd ../schema 
IF NOT EXIST output mkdir output
gcc -o schema_tool.exe -g -O2 ../src/schema_tool.c -lm
schema_tool.exe kotk_login_udp_11.schm output/kotk_login_udp_11.c

popd
pushd login_binaries
gcc -shared -o loginModule.dll -g -O2 ../src/kotk_login_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm
gcc -o loginServer.exe -g -O2 ../src/win32_login_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm

popd
