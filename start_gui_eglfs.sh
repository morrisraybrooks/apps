#!/bin/bash

# Vacuum Controller GUI Startup Script - EGLFS Version
# This script starts the vacuum controller with direct hardware rendering

echo "Starting Vacuum Controller GUI with EGLFS..."

# Check if build exists
if [ ! -f "./build/VacuumController" ]; then
    echo "Error: VacuumController executable not found"
    echo "Please run: ./build.sh first"
    exit 1
fi

# Check if user is in required groups for hardware access
if ! groups | grep -q "gpio"; then
    echo "Warning: User not in gpio group - may need sudo for hardware access"
fi

if ! groups | grep -q "spi"; then
    echo "Warning: User not in spi group - may need sudo for hardware access"
fi

# Set display configuration for Wayland
echo "Configuring for Wayland display..."

# Configure Qt to use the Wayland platform plugin
export QT_QPA_PLATFORM=wayland
export QT_QPA_FONTDIR=/usr/share/fonts

echo "Environment configured:"
echo "  Platform: Wayland"
echo "  Hardware Access: Via user groups (gpio, spi)"

# Start the vacuum controller
echo "Launching Vacuum Controller..."
cd "$(dirname "$0")"

# Run with Wayland
QT_QPA_PLATFORM=wayland ./build/VacuumController

echo "Vacuum Controller has exited."
