#!/bin/bash
#===============================================================================
# V-Contour System - Raspberry Pi 4 Quick Setup Script
# One-command installation for production deployment
#===============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
REPO_URL="https://github.com/morrisraybrooks/apps.git"
INSTALL_DIR="/opt/v-contour"
BUILD_DIR="$INSTALL_DIR/build"

log() { echo -e "${BLUE}[V-CONTOUR]${NC} $1"; }
success() { echo -e "${GREEN}[✓]${NC} $1"; }
warn() { echo -e "${YELLOW}[!]${NC} $1"; }
error() { echo -e "${RED}[✗]${NC} $1"; exit 1; }

print_banner() {
    echo -e "${PURPLE}"
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║           V-CONTOUR SYSTEM - RASPBERRY PI 4 SETUP            ║"
    echo "║                  Automated Installation Script               ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

check_raspberry_pi() {
    log "Checking system..."
    if ! grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null; then
        warn "Not running on Raspberry Pi - some features may not work"
    fi
    
    if [[ $EUID -eq 0 ]]; then
        error "Do not run as root. The script will use sudo when needed."
    fi
    success "System check passed"
}

enable_interfaces() {
    log "Enabling hardware interfaces (SPI, I2C, GPIO)..."
    
    # Enable SPI
    if ! grep -q "^dtparam=spi=on" /boot/config.txt 2>/dev/null; then
        sudo bash -c 'echo "dtparam=spi=on" >> /boot/config.txt'
        success "SPI enabled"
    else
        success "SPI already enabled"
    fi
    
    # Enable I2C
    if ! grep -q "^dtparam=i2c_arm=on" /boot/config.txt 2>/dev/null; then
        sudo bash -c 'echo "dtparam=i2c_arm=on" >> /boot/config.txt'
        success "I2C enabled"
    else
        success "I2C already enabled"
    fi
}

install_dependencies() {
    log "Installing system dependencies..."
    sudo apt-get update
    
    # Core build tools
    sudo apt-get install -y \
        build-essential cmake git pkg-config \
        qtbase5-dev qtcharts5-dev libqt5charts5-dev \
        libgpiod-dev libgpiod2 gpiod
    
    success "Dependencies installed"
}

setup_user_permissions() {
    log "Setting up user permissions for GPIO/SPI access..."
    
    # Add user to required groups
    sudo usermod -aG gpio,spi,i2c $USER 2>/dev/null || true
    
    # Create udev rules for GPIO access without sudo
    sudo tee /etc/udev/rules.d/99-gpio.rules > /dev/null << 'EOF'
SUBSYSTEM=="gpio", KERNEL=="gpiochip*", GROUP="gpio", MODE="0660"
SUBSYSTEM=="spidev", GROUP="spi", MODE="0660"
EOF
    
    sudo udevadm control --reload-rules
    sudo udevadm trigger
    
    success "User permissions configured"
}

clone_and_build() {
    log "Cloning repository and building..."
    
    sudo mkdir -p $INSTALL_DIR
    sudo chown $USER:$USER $INSTALL_DIR
    
    if [[ -d "$INSTALL_DIR/.git" ]]; then
        cd $INSTALL_DIR
        git pull origin main
    else
        git clone $REPO_URL $INSTALL_DIR
        cd $INSTALL_DIR
    fi
    
    mkdir -p $BUILD_DIR && cd $BUILD_DIR
    cmake .. -DCMAKE_BUILD_TYPE=Release -DRASPBERRY_PI_BUILD=ON
    make -j3  # Use -j3 for Pi 4 memory optimization
    
    success "Build complete"
}

install_system() {
    log "Installing system-wide..."
    cd $BUILD_DIR
    sudo make install
    sudo ldconfig
    success "System installation complete"
}

create_launcher() {
    log "Creating desktop launcher..."
    
    sudo tee /usr/local/bin/v-contour << 'EOF'
#!/bin/bash
# V-Contour Launcher
export QT_QPA_PLATFORM=wayland
export QT_SCALE_FACTOR=1.5
cd /opt/v-contour/build
exec ./VacuumController "$@"
EOF
    
    sudo chmod +x /usr/local/bin/v-contour
    success "Launcher created: v-contour"
}

create_systemd_service() {
    log "Creating systemd service for auto-start..."
    
    sudo tee /etc/systemd/system/v-contour.service > /dev/null << EOF
[Unit]
Description=V-Contour Vacuum Controller
After=graphical-session.target

[Service]
Type=simple
User=$USER
Environment=DISPLAY=:0
Environment=QT_QPA_PLATFORM=wayland
WorkingDirectory=/opt/v-contour/build
ExecStart=/opt/v-contour/build/VacuumController
Restart=on-failure
RestartSec=5

[Install]
WantedBy=graphical-session.target
EOF
    
    sudo systemctl daemon-reload
    success "Systemd service created (not enabled by default)"
}

print_summary() {
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║              INSTALLATION COMPLETE!                          ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${CYAN}Quick Commands:${NC}"
    echo "  v-contour              # Run the application"
    echo "  sudo v-contour         # Run with GPIO access (if not in gpio group)"
    echo ""
    echo -e "${CYAN}Auto-start (optional):${NC}"
    echo "  sudo systemctl enable v-contour  # Enable auto-start"
    echo "  sudo systemctl start v-contour   # Start service now"
    echo ""
    echo -e "${CYAN}Documentation:${NC}"
    echo "  https://morrisraybrooks.github.io/apps/"
    echo ""
    echo -e "${YELLOW}⚠️  IMPORTANT: Reboot to apply GPIO permissions!${NC}"
    echo ""
    read -p "Reboot now? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo reboot
    fi
}

# Main execution
main() {
    print_banner
    check_raspberry_pi
    enable_interfaces
    install_dependencies
    setup_user_permissions
    clone_and_build
    install_system
    create_launcher
    create_systemd_service
    print_summary
}

main "$@"

