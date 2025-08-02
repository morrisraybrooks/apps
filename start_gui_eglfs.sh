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

# Set display configuration for direct hardware rendering
echo "Configuring EGLFS for 50-inch HDMI display..."

# Configure Qt for direct hardware rendering (no X11/Wayland needed)
export QT_QPA_PLATFORM=eglfs
export QT_QPA_EGLFS_ALWAYS_SET_MODE=1
export QT_QPA_EGLFS_HIDECURSOR=0
export QT_QPA_FONTDIR=/usr/share/fonts
export QT_QPA_GENERIC_PLUGINS=evdevmouse,evdevkeyboard
export QT_QPA_EVDEV_MOUSE_PARAMETERS=/dev/input/event0

# Optional: Force specific display if multiple outputs
# export QT_QPA_EGLFS_KMS_CONFIG=/dev/dri/card1

echo "Environment configured:"
echo "  Platform: EGLFS (Direct Hardware Rendering)"
echo "  Display: Full Screen on Primary Output"
echo "  Hardware Access: Via user groups (gpio, spi)"

# Start the vacuum controller
echo "Launching Vacuum Controller..."
cd "$(dirname "$0")"

# Run with EGLFS for direct hardware rendering
QT_QPA_PLATFORM=eglfs QT_QPA_EGLFS_ALWAYS_SET_MODE=1 QT_QPA_EGLFS_HIDECURSOR=0 QT_QPA_GENERIC_PLUGINS=evdevmouse,evdevkeyboard ./build/VacuumController

echo "Vacuum Controller has exited."
