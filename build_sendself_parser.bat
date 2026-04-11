@echo off

IF NOT EXIST zone_binaries mkdir zone_binaries
pushd zone_binaries

gcc -o sendself_parser.exe -g -O2 ../src/sendself_bin_parser.c -lm

popd
