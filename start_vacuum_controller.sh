#!/bin/bash

# Vacuum Controller Startup Script
# This script ensures proper hardware access and Wayland display permissions

echo "Starting Vacuum Controller with full hardware access..."

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo "Running as root - hardware access enabled"
else
    echo "Restarting with sudo for hardware access..."
    exec sudo -E "$0" "$@"
fi

# Preserve Wayland environment for GUI
export XDG_RUNTIME_DIR="/run/user/1000"
export WAYLAND_DISPLAY="wayland-0"

# Set proper permissions for Wayland socket
chmod 755 /run/user/1000
chmod 755 /run/user/1000/wayland-0 2>/dev/null || true

# Change to the build directory
cd "$(dirname "$0")/build" || {
    echo "Error: Could not find build directory"
    exit 1
}

echo "Starting Vacuum Controller GUI..."
echo "Hardware: ENABLED"
echo "Display: Wayland"
echo "User: root (for hardware access)"
echo ""

# Run the vacuum controller
exec ./VacuumController
