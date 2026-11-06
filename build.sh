#!/bin/bash
bear -- clang++ -std=c++23 -g -O0 -fno-omit-frame-pointer -DNOMINMAX -I $(pwd) $(find ./ -type f -name "*cpp") $(pkg-config --cflags --libs raylib) -o exe


