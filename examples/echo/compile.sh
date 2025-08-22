#! /bin/bash

FLAGS="-march=x86-64-v3 -std=c++2c -fno-exceptions -fno-rtti -mavx2 -mbmi -mbmi2 -I../../src -I/nix/store/gbwf7by78vgqjmcjf1smbh1zib0mycf8-libcxx-20.1.6-dev/include -I/nix/store/qqjsw1fzand71ajx326r84zgi485mqb8-liburing-2.11-dev/include"
rm -Rf ./build
mkdir -p build
echo "Compiling echo server"
clang++ -O1 -luring $FLAGS -I../../src -l../../build/ak.a -o ./build/echo-server ./src/server.cc
echo "Compiling echo client"
clang++ -O1 -luring $FLAGS -I../../src -l../../build/ak.a -o ./build/echo-client ./src/client.cc  

