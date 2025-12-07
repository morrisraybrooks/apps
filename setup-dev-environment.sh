#!/bin/bash

# Development Environment Setup Script for Vacuum Controller GUI System
# This script sets up a complete development environment with all tools and dependencies

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
INSTALL_OPTIONAL_TOOLS="true"
SETUP_GIT_HOOKS="true"
CONFIGURE_IDE="true"

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --no-optional-tools    Skip installation of optional development tools"
    echo "  --no-git-hooks         Skip Git hooks setup"
    echo "  --no-ide-config        Skip IDE configuration"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "This script will install:"
    echo "  - Build dependencies (CMake, Qt5, libgpiod)"
    echo "  - Development tools (clang-format, clang-tidy, cppcheck)"
    echo "  - Documentation tools (Doxygen, Graphviz)"
    echo "  - Testing tools (Valgrind, lcov)"
    echo "  - Git hooks for code quality"
    echo "  - IDE configuration files"
}

check_system() {
    log_info "Checking system compatibility..."
    
    # Check if running on supported system
    if [[ ! -f /etc/debian_version ]]; then
        log_error "This script is designed for Debian/Ubuntu systems"
        exit 1
    fi
    
    # Check if running as root
    if [[ $EUID -eq 0 ]]; then
        log_error "This script should not be run as root"
        exit 1
    fi
    
    log_success "System compatibility check passed"
}

update_system() {
    log_info "Updating system package lists..."
    sudo apt-get update
    log_success "System updated"
}

install_build_dependencies() {
    log_info "Installing build dependencies..."
    
    local packages=(
        # Build tools
        "build-essential"
        "cmake"
        "ninja-build"
        "pkg-config"
        "git"
        
        # Qt5 development
        "qtbase5-dev"
        "qtcharts5-dev"
        "qt5-qmake"
        "qtbase5-dev-tools"
        
        # Hardware libraries
        "libgpiod-dev"
        "libgpiod2"
        
        # System libraries
        "libc6-dev"
        "libstdc++-dev"
    )
    
    sudo apt-get install -y "${packages[@]}"
    log_success "Build dependencies installed"
}

install_development_tools() {
    log_info "Installing development tools..."
    
    local packages=(
        # Code analysis
        "clang-format"
        "clang-tidy"
        "cppcheck"
        "clang"
        
        # Testing and debugging
        "valgrind"
        "gdb"
        "lcov"
        
        # Documentation
        "doxygen"
        "graphviz"
        
        # Utilities
        "tree"
        "htop"
        "curl"
        "wget"
    )
    
    sudo apt-get install -y "${packages[@]}"
    log_success "Development tools installed"
}

install_optional_tools() {
    if [[ "$INSTALL_OPTIONAL_TOOLS" != "true" ]]; then
        log_info "Skipping optional tools installation"
        return 0
    fi
    
    log_info "Installing optional development tools..."
    
    local packages=(
        # Additional analysis tools
        "iwyu"  # Include What You Use
        "ccache"  # Compiler cache
        
        # Performance tools
        "perf-tools-unstable"
        "linux-perf"
        
        # Additional utilities
        "jq"  # JSON processor
        "xmlstarlet"  # XML processor
        "pandoc"  # Document converter
    )
    
    # Install packages that are available
    for package in "${packages[@]}"; do
        if apt-cache show "$package" >/dev/null 2>&1; then
            sudo apt-get install -y "$package" || log_warning "Failed to install $package"
        else
            log_warning "Package $package not available"
        fi
    done
    
    log_success "Optional tools installation completed"
}

setup_git_hooks() {
    if [[ "$SETUP_GIT_HOOKS" != "true" ]]; then
        log_info "Skipping Git hooks setup"
        return 0
    fi
    
    log_info "Setting up Git hooks..."
    
    # Create hooks directory
    mkdir -p .git/hooks
    
    # Pre-commit hook for code formatting
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
# Pre-commit hook for code formatting

# Check if clang-format is available
if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found, skipping formatting check"
    exit 0
fi

# Get list of staged C++ files
staged_files=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h|hpp|cc|cxx)$' || true)

if [ -z "$staged_files" ]; then
    exit 0
fi

# Check formatting
format_issues=false
for file in $staged_files; do
    if [ -f "$file" ]; then
        if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
            echo "Formatting issues found in: $file"
            format_issues=true
        fi
    fi
done

if [ "$format_issues" = true ]; then
    echo ""
    echo "Code formatting issues found. Please run:"
    echo "  find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i"
    echo ""
    exit 1
fi

exit 0
EOF

    # Pre-push hook for running tests
    cat > .git/hooks/pre-push << 'EOF'
#!/bin/bash
# Pre-push hook for running tests

echo "Running tests before push..."

# Build and run tests
if [ -d "build" ]; then
    cd build
    if make -j$(nproc) && ctest --output-on-failure; then
        echo "All tests passed"
        exit 0
    else
        echo "Tests failed, push aborted"
        exit 1
    fi
else
    echo "No build directory found, skipping tests"
    exit 0
fi
EOF

    # Make hooks executable
    chmod +x .git/hooks/pre-commit
    chmod +x .git/hooks/pre-push
    
    log_success "Git hooks configured"
}

configure_ide() {
    if [[ "$CONFIGURE_IDE" != "true" ]]; then
        log_info "Skipping IDE configuration"
        return 0
    fi
    
    log_info "Setting up IDE configuration..."
    
    # Create .vscode directory for VS Code configuration
    mkdir -p .vscode
    
    # VS Code settings
    cat > .vscode/settings.json << 'EOF'
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.cppStandard": "c++17",
    "C_Cpp.default.compilerPath": "/usr/bin/g++",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/src/**",
        "/usr/include/qt5/**"
    ],
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.generator": "Ninja",
    "files.associations": {
        "*.h": "cpp",
        "*.cpp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.clang_format_style": "file",
    "C_Cpp.clang_format_fallbackStyle": "Google"
}
EOF

    # VS Code tasks
    cat > .vscode/tasks.json << 'EOF'
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Debug",
            "type": "shell",
            "command": "./build.sh",
            "args": ["-t", "Debug", "-c"],
            "group": "build",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        },
        {
            "label": "Build Release",
            "type": "shell",
            "command": "./build.sh",
            "args": ["-t", "Release", "-c"],
            "group": {
                "kind": "build",
                "isDefault": true
            }
        },
        {
            "label": "Run Tests",
            "type": "shell",
            "command": "./build.sh",
            "args": ["-r"],
            "group": "test"
        },
        {
            "label": "Format Code",
            "type": "shell",
            "command": "find",
            "args": ["src", "-name", "*.cpp", "-o", "-name", "*.h", "|", "xargs", "clang-format", "-i"],
            "group": "build"
        }
    ]
}
EOF

    # VS Code launch configuration
    cat > .vscode/launch.json << 'EOF'
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Vacuum Controller",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/VacuumController",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build",
            "environment": [
                {
                    "name": "QT_QPA_PLATFORM",
                    "value": "xcb"
                }
            ],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build Debug"
        }
    ]
}
EOF

    # Create clang-format configuration
    cat > .clang-format << 'EOF'
BasedOnStyle: Google
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 120
AccessModifierOffset: -2
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignOperands: true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: None
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: true
AlwaysBreakTemplateDeclarations: true
BinPackArguments: true
BinPackParameters: true
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: false
BreakStringLiterals: true
CommentPragmas: '^ IWYU pragma:'
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: true
DisableFormat: false
ExperimentalAutoDetectBinPacking: false
ForEachMacros: [ foreach, Q_FOREACH, BOOST_FOREACH ]
IncludeCategories:
  - Regex:           '^<.*\.h>'
    Priority:        1
  - Regex:           '^<.*'
    Priority:        2
  - Regex:           '.*'
    Priority:        3
IndentCaseLabels: true
IndentWrappedFunctionNames: false
KeepEmptyLinesAtTheStartOfBlocks: false
MacroBlockBegin: ''
MacroBlockEnd: ''
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
PenaltyBreakBeforeFirstCallParameter: 1
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 200
PointerAlignment: Left
ReflowComments: true
SortIncludes: true
SpaceAfterCStyleCast: false
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 2
SpacesInAngles: false
SpacesInContainerLiterals: true
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard: Cpp11
EOF

    log_success "IDE configuration completed"
}

create_build_script() {
    log_info "Creating enhanced build script..."
    
    # The build script was already created in the previous implementation
    if [[ ! -f "build.sh" ]]; then
        log_warning "build.sh not found, creating basic version..."
        cat > build.sh << 'EOF'
#!/bin/bash
# Basic build script - see DEVELOPMENT_PRACTICES.md for full version
set -e
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
EOF
        chmod +x build.sh
    fi
    
    chmod +x build.sh
    log_success "Build script ready"
}

verify_installation() {
    log_info "Verifying installation..."
    
    local tools=(
        "cmake"
        "make"
        "g++"
        "clang-format"
        "clang-tidy"
        "cppcheck"
        "doxygen"
        "valgrind"
        "lcov"
    )
    
    local missing_tools=()
    for tool in "${tools[@]}"; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            missing_tools+=("$tool")
        fi
    done
    
    if [[ ${#missing_tools[@]} -eq 0 ]]; then
        log_success "All development tools are available"
    else
        log_warning "Missing tools: ${missing_tools[*]}"
    fi
    
    # Check Qt5
    if pkg-config --exists Qt5Core Qt5Widgets Qt5Charts; then
        log_success "Qt5 development libraries found"
    else
        log_error "Qt5 development libraries not found"
    fi
    
    # Check libgpiod
    if pkg-config --exists libgpiod; then
        log_success "libgpiod library found"
    else
        log_error "libgpiod library not found"
    fi
}

print_next_steps() {
    echo ""
    echo "=================================="
    echo "    DEVELOPMENT ENVIRONMENT READY"
    echo "=================================="
    echo ""
    echo "Next steps:"
    echo "1. Build the project:"
    echo "   ./build.sh -c -r"
    echo ""
    echo "2. Install the application:"
    echo "   ./build.sh -c -i"
    echo ""
    echo "3. Run tests:"
    echo "   cd build && ctest"
    echo ""
    echo "4. Generate documentation:"
    echo "   cd build && make docs"
    echo ""
    echo "5. Create packages:"
    echo "   cd build && cpack -G DEB"
    echo ""
    echo "Development tools available:"
    echo "- Code formatting: find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i"
    echo "- Static analysis: cppcheck --enable=all src/"
    echo "- Memory checking: valgrind ./build/VacuumController"
    echo "- Version management: ./scripts/version.sh --help"
    echo ""
    echo "IDE configuration created for VS Code (.vscode/)"
    echo "Git hooks configured for code quality"
    echo ""
    log_success "Development environment setup completed!"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-optional-tools)
            INSTALL_OPTIONAL_TOOLS="false"
            shift
            ;;
        --no-git-hooks)
            SETUP_GIT_HOOKS="false"
            shift
            ;;
        --no-ide-config)
            CONFIGURE_IDE="false"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Main execution
main() {
    log_info "Setting up Vacuum Controller development environment..."
    
    check_system
    update_system
    install_build_dependencies
    install_development_tools
    install_optional_tools
    setup_git_hooks
    configure_ide
    create_build_script
    verify_installation
    print_next_steps
}

# Execute main function
main "$@"
