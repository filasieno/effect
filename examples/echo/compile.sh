
rm -Rf ./build
mkdir -p build
echo "Compiling echo server"
clang++ -O1 -luring -std=c++2c -fno-exceptions -fno-rtti -I../../src -o ./build/echo-server ./src/server.cc
echo "Compiling echo client"
clang++ -O1 -luring -std=c++2c -fno-exceptions -fno-rtti -I../../src -o ./build/echo-client ./src/client.cc  