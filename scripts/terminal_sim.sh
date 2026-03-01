#!/bin/bash

#echo "=== Killing terminal_sim if running ==="
#pkill -f "terminal_sim" 2>/dev/null || true

echo "Building terminal_sim..."
clang++ -std=c++17 -DNOMINMAX -I./src -o ./scripts/terminal_sim ./src/terminal_main.cpp
if [[ $? -ne 0 ]]; then
    echo "Build failed!"
    exit 1
fi

echo "Running terminal simulation..."
./scripts/terminal_sim
read -p "Press Enter to exit..."

