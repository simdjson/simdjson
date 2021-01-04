#!/bin/sh
rm ./main
g++ -I../include -I../src -Isrc -O3 -m64 ./main.cpp -o ./main
./main
