#!/bin/bash

# Vacuum Controller Build Script
# This script builds the vacuum controller GUI application

set -e  # Exit on any error

echo "=== Vacuum Controller Build Script ==="
echo "Building GUI application for Raspberry Pi..."

# Check if we're on Raspberry Pi
if [[ $(uname -m) == "armv7l" ]] || [[ $(uname -m) == "aarch64" ]]; then
    echo "Detected ARM architecture - building for Raspberry Pi"
    ON_RPI=true
else
    echo "Detected x86 architecture - building for development/testing"
    ON_RPI=false
fi

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring build with CMake..."

# Configure CMake based on platform
if [ "$ON_RPI" = true ]; then
    # Raspberry Pi specific configuration
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_C_COMPILER=gcc \
        -DCMAKE_INSTALL_PREFIX=/usr/local
else
    # Development/testing configuration
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

echo "Building application..."
make -j$(nproc)

echo "Build completed successfully!"

# Check if executable was created
if [ -f "VacuumController" ]; then
    echo "Executable created: VacuumController"
    echo "File size: $(du -h VacuumController | cut -f1)"
    
    if [ "$ON_RPI" = true ]; then
        echo ""
        echo "To install the application system-wide, run:"
        echo "  sudo make install"
        echo ""
        echo "To run the application:"
        echo "  ./VacuumController"
        echo ""
        echo "Note: The application requires root privileges to access GPIO pins."
        echo "Run with: sudo ./VacuumController"
    else
        echo ""
        echo "Development build completed."
        echo "Note: Hardware features will not work on non-Raspberry Pi systems."
        echo "Run with: ./VacuumController"
    fi
else
    echo "ERROR: Executable not found!"
    exit 1
fi

echo ""
echo "=== Build Summary ==="
echo "Platform: $(uname -m)"
echo "Build type: $(if [ "$ON_RPI" = true ]; then echo "Release (Raspberry Pi)"; else echo "Debug (Development)"; fi)"
echo "Build directory: $(pwd)"
echo "=== Build Complete ==="
