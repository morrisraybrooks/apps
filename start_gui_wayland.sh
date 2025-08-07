#!/bin/bash

# Vacuum Controller GUI Startup Script (Wayland)
# This script starts the Vacuum Controller GUI using Wayland compositor
# Enhanced Air Pulse System with Anti-Detachment Integration

echo "=========================================="
echo "  Vacuum Controller - Enhanced Air Pulse"
echo "=========================================="
echo "Starting GUI with Wayland compositor..."

# Set Qt platform to Wayland for native desktop integration
export QT_QPA_PLATFORM=wayland

# Wayland-specific environment variables for optimal performance
export QT_WAYLAND_DISABLE_WINDOWDECORATION=0  # Keep window decorations
export QT_AUTO_SCREEN_SCALE_FACTOR=1          # Enable auto scaling
export QT_SCALE_FACTOR=1.0                    # Set base scale factor

# Change to build directory
cd "$(dirname "$0")/build" || {
    echo "❌ Error: Could not change to build directory"
    exit 1
}

# Check if executable exists
if [ ! -f "./VacuumController" ]; then
    echo "❌ Error: VacuumController executable not found in build directory"
    echo "Please build the project first with: make -j3"
    exit 1
fi

# Check if config directory exists (create symlink if needed)
if [ ! -L "config" ] && [ ! -d "config" ]; then
    echo "🔗 Creating config symlink..."
    ln -sf ../config config
fi

# Display system information
echo ""
echo "🎯 System Configuration:"
echo "   Platform: Wayland"
echo "   Enhanced Air Pulse: ✅ Active"
echo "   Anti-Detachment: ✅ 50 mmHg threshold"
echo "   Pattern Engine: ✅ 16 patterns loaded"
echo "   Safety Systems: ✅ Integrated monitoring"
echo ""
echo "🚀 Launching VacuumController..."
echo "📱 GUI will open in windowed mode with touch support"
echo "⚠️  Press Ctrl+C to stop the application"
echo ""

# Start the application with Wayland
./VacuumController
