#!/usr/bin/env bash
set -e  # Stop on error

echo "=== DEBUG SCRIPT STARTED ==="
echo "PWD: $(pwd)"

root="$HOME/development/PixelWorldCpp"
exe="$HOME/development/PixelWorldCpp/godot-executable-4-4-linux/Godot_v4.4.1-stable_linux.x86_64"
proj="$HOME/development/PixelWorldCpp/project"

# Validate paths
if [[ ! -f "$exe" ]]; then
    echo "ERROR: Godot executable NOT FOUND: $exe"
    exit 1
fi

if [[ ! -d "$proj" ]]; then
    echo "ERROR: Project path NOT FOUND: $proj"
    exit 1
fi

echo "=== Killing running Godot (if any) ==="
pkill -f "Godot_v4.4.1-stable" 2>/dev/null || true

cd "$root"

echo "=== Building GDExtension (Debug) ==="
scons dev_build=yes
if [[ $? -ne 0 ]]; then
    echo "SCons build failed. Godot will NOT launch."
    exit 1
fi

echo "=== Launching Godot ==="
"$exe" --path "$proj" &

echo "=== DEBUG SCRIPT FINISHED ==="


