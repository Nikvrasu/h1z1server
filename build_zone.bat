@echo off

IF NOT EXIST zone_binaries mkdir zone_binaries
pushd zone_binaries

cd ../schema 
IF NOT EXIST output mkdir output
gcc -o schema_tool.exe -g -O2 ../src/schema_tool.c -lm
schema_tool.exe client_protocol_1087.schm output/client_protocol_1087.c

popd
pushd zone_binaries
gcc -shared -o zoneModule.dll -g -O0 ../src/kotk_zone_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm
gcc -o zoneServer.exe -g -O2 ../src/win32_zone_server.c -DYOTE_INTERNAL -luser32 -lkernel32 -lws2_32 -lwinmm

popd
