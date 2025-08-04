echo "Compiling echo example"
rm -Rf ./build
mkdir -p build
clang++ -O1 -luring -std=c++2c -fno-exceptions -fno-rtti -I../../src -o ./build/echo ./src/main.cc