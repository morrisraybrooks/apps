#!/bin/bash
# Version management script for Vacuum Controller

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VERSION_FILE="$PROJECT_ROOT/VERSION"
CMAKE_FILE="$PROJECT_ROOT/CMakeLists.txt"
CHANGELOG_FILE="$PROJECT_ROOT/CHANGELOG.md"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

usage() {
    echo "Usage: $0 <command> [options]"
    echo ""
    echo "Commands:"
    echo "  current                    Show current version"
    echo "  bump <major|minor|patch>   Bump version number"
    echo "  set <version>              Set specific version (e.g., 1.2.3)"
    echo "  tag                        Create git tag for current version"
    echo "  changelog                  Generate changelog from git commits"
    echo "  release <version>          Full release process (bump, tag, changelog)"
    echo ""
    echo "Examples:"
    echo "  $0 current"
    echo "  $0 bump minor"
    echo "  $0 set 2.0.0"
    echo "  $0 release 1.1.0"
}

get_current_version() {
    if [ -f "$VERSION_FILE" ]; then
        cat "$VERSION_FILE"
    else
        # Extract from CMakeLists.txt
        grep "project.*VERSION" "$CMAKE_FILE" | sed -n 's/.*VERSION \([0-9.]*\).*/\1/p'
    fi
}

set_version() {
    local new_version="$1"
    
    # Validate version format
    if ! [[ "$new_version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo -e "${RED}Error: Invalid version format. Use semantic versioning (e.g., 1.2.3)${NC}"
        exit 1
    fi
    
    # Update VERSION file
    echo "$new_version" > "$VERSION_FILE"
    
    # Update CMakeLists.txt
    sed -i "s/project(VacuumController VERSION [0-9.]*/project(VacuumController VERSION $new_version/" "$CMAKE_FILE"
    
    # Update package.json if it exists (for potential web components)
    if [ -f "$PROJECT_ROOT/package.json" ]; then
        sed -i "s/\"version\": \"[0-9.]*\"/\"version\": \"$new_version\"/" "$PROJECT_ROOT/package.json"
    fi
    
    echo -e "${GREEN}Version updated to $new_version${NC}"
}

bump_version() {
    local bump_type="$1"
    local current_version
    current_version=$(get_current_version)
    
    if [ -z "$current_version" ]; then
        echo -e "${RED}Error: Could not determine current version${NC}"
        exit 1
    fi
    
    IFS='.' read -r major minor patch <<< "$current_version"
    
    case "$bump_type" in
        major)
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        minor)
            minor=$((minor + 1))
            patch=0
            ;;
        patch)
            patch=$((patch + 1))
            ;;
        *)
            echo -e "${RED}Error: Invalid bump type. Use major, minor, or patch${NC}"
            exit 1
            ;;
    esac
    
    local new_version="$major.$minor.$patch"
    set_version "$new_version"
}

create_git_tag() {
    local version
    version=$(get_current_version)
    
    if [ -z "$version" ]; then
        echo -e "${RED}Error: Could not determine current version${NC}"
        exit 1
    fi
    
    local tag_name="v$version"
    
    # Check if tag already exists
    if git tag -l | grep -q "^$tag_name$"; then
        echo -e "${YELLOW}Warning: Tag $tag_name already exists${NC}"
        return 0
    fi
    
    # Create annotated tag
    git tag -a "$tag_name" -m "Release version $version"
    echo -e "${GREEN}Created git tag: $tag_name${NC}"
    
    echo -e "${BLUE}To push the tag, run: git push origin $tag_name${NC}"
}

generate_changelog() {
    local version
    version=$(get_current_version)
    
    if [ -z "$version" ]; then
        echo -e "${RED}Error: Could not determine current version${NC}"
        exit 1
    fi
    
    local temp_changelog="/tmp/changelog_new.md"
    local date
    date=$(date +%Y-%m-%d)
    
    # Get the last tag
    local last_tag
    last_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
    
    # Generate changelog header
    {
        echo "# Changelog"
        echo ""
        echo "All notable changes to this project will be documented in this file."
        echo ""
        echo "The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),"
        echo "and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)."
        echo ""
        echo "## [Unreleased]"
        echo ""
        echo "## [$version] - $date"
        echo ""
    } > "$temp_changelog"
    
    # Generate changelog content from git commits
    if [ -n "$last_tag" ]; then
        echo "### Added" >> "$temp_changelog"
        git log --pretty=format:"- %s" "$last_tag"..HEAD --grep="^feat" --grep="^add" >> "$temp_changelog" || true
        echo "" >> "$temp_changelog"
        
        echo "### Changed" >> "$temp_changelog"
        git log --pretty=format:"- %s" "$last_tag"..HEAD --grep="^change" --grep="^update" >> "$temp_changelog" || true
        echo "" >> "$temp_changelog"
        
        echo "### Fixed" >> "$temp_changelog"
        git log --pretty=format:"- %s" "$last_tag"..HEAD --grep="^fix" --grep="^bug" >> "$temp_changelog" || true
        echo "" >> "$temp_changelog"
        
        echo "### Security" >> "$temp_changelog"
        git log --pretty=format:"- %s" "$last_tag"..HEAD --grep="^security" >> "$temp_changelog" || true
        echo "" >> "$temp_changelog"
    else
        echo "### Added" >> "$temp_changelog"
        echo "- Initial release" >> "$temp_changelog"
        echo "" >> "$temp_changelog"
    fi
    
    # Append existing changelog if it exists
    if [ -f "$CHANGELOG_FILE" ] && [ -s "$CHANGELOG_FILE" ]; then
        # Skip the header of existing changelog
        tail -n +8 "$CHANGELOG_FILE" >> "$temp_changelog"
    fi
    
    # Replace the original changelog
    mv "$temp_changelog" "$CHANGELOG_FILE"
    
    echo -e "${GREEN}Changelog updated for version $version${NC}"
}

full_release() {
    local version="$1"
    
    if [ -z "$version" ]; then
        echo -e "${RED}Error: Version required for release${NC}"
        usage
        exit 1
    fi
    
    echo -e "${BLUE}Starting release process for version $version${NC}"
    
    # Check if working directory is clean
    if ! git diff-index --quiet HEAD --; then
        echo -e "${RED}Error: Working directory is not clean. Commit or stash changes first.${NC}"
        exit 1
    fi
    
    # Set version
    set_version "$version"
    
    # Generate changelog
    generate_changelog
    
    # Commit version changes
    git add "$VERSION_FILE" "$CMAKE_FILE" "$CHANGELOG_FILE"
    git commit -m "chore: bump version to $version"
    
    # Create tag
    create_git_tag
    
    echo -e "${GREEN}Release $version completed successfully!${NC}"
    echo -e "${BLUE}Next steps:${NC}"
    echo "1. Review the changes: git log --oneline -5"
    echo "2. Push changes: git push origin main"
    echo "3. Push tag: git push origin v$version"
    echo "4. Create GitHub release from the tag"
}

# Main script logic
case "${1:-}" in
    current)
        version=$(get_current_version)
        if [ -n "$version" ]; then
            echo -e "${GREEN}Current version: $version${NC}"
        else
            echo -e "${RED}Could not determine current version${NC}"
            exit 1
        fi
        ;;
    bump)
        if [ -z "${2:-}" ]; then
            echo -e "${RED}Error: Bump type required${NC}"
            usage
            exit 1
        fi
        bump_version "$2"
        ;;
    set)
        if [ -z "${2:-}" ]; then
            echo -e "${RED}Error: Version required${NC}"
            usage
            exit 1
        fi
        set_version "$2"
        ;;
    tag)
        create_git_tag
        ;;
    changelog)
        generate_changelog
        ;;
    release)
        if [ -z "${2:-}" ]; then
            echo -e "${RED}Error: Version required for release${NC}"
            usage
            exit 1
        fi
        full_release "$2"
        ;;
    *)
        usage
        exit 1
        ;;
esac
