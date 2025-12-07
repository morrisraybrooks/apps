# Contributing to Vacuum Controller GUI System

Thank you for your interest in contributing to the Vacuum Controller GUI System! This document provides guidelines for contributing to this medical device software project.

## Important Notice

⚠️ **MEDICAL DEVICE SOFTWARE WARNING** ⚠️

This software is designed for medical device applications. All contributions must maintain the highest standards of safety, reliability, and code quality. Any changes that could affect patient safety require extensive testing and validation.

## Code of Conduct

This project adheres to a code of conduct that promotes a welcoming and inclusive environment. By participating, you agree to uphold these standards.

## How to Contribute

### Reporting Issues

1. **Search existing issues** first to avoid duplicates
2. **Use the issue template** when creating new issues
3. **Provide detailed information** including:
   - Steps to reproduce
   - Expected vs actual behavior
   - System configuration
   - Log files (if applicable)
   - Screenshots (for UI issues)

### Submitting Changes

1. **Fork the repository**
2. **Create a feature branch** from `main`
3. **Make your changes** following our coding standards
4. **Add tests** for new functionality
5. **Update documentation** as needed
6. **Submit a pull request**

## Development Setup

### Prerequisites

```bash
# Install dependencies
sudo apt update
sudo apt install qt6-base-dev qt6-charts-dev cmake build-essential

# Install development tools
sudo apt install clang-format cppcheck clang-tidy valgrind
```

### Building the Project

```bash
# Clone the repository
git clone https://github.com/morrisraybrooks/apps.git
cd vacuum-controller

# Build the project
mkdir build && cd build
cmake ..
make -j4

# Run tests
make run_all_tests
```

## Coding Standards

### C++ Guidelines

- **Standard**: C++17
- **Style**: Follow the existing code style
- **Formatting**: Use clang-format with the provided configuration
- **Naming**: Use descriptive names for variables and functions

### Code Quality Requirements

1. **All code must compile without warnings**
2. **All new code must have unit tests**
3. **Code coverage must not decrease**
4. **Static analysis must pass (cppcheck, clang-tidy)**
5. **Memory leaks are not acceptable (valgrind)**

### Safety-Critical Code

For safety-critical components (anything in `src/safety/`):

1. **Extra scrutiny required** - all changes must be reviewed by maintainers
2. **Comprehensive testing** - unit tests, integration tests, and safety validation
3. **Documentation updates** - all safety changes must be documented
4. **Performance validation** - timing requirements must be maintained

## Testing Requirements

### Test Categories

1. **Unit Tests** - Test individual components
2. **Integration Tests** - Test component interactions
3. **Safety Tests** - Validate safety-critical functionality
4. **Performance Tests** - Verify real-time requirements
5. **UI Tests** - Validate user interface functionality

### Running Tests

```bash
# Run all tests
make run_all_tests

# Run specific test suites
make run_safety_tests
make run_hardware_tests
make run_pattern_tests
make run_gui_tests
make run_performance_tests
make run_integration_tests

# Generate test report
make test_report

# Check test coverage
make coverage
```

### Test Requirements

- **All new features must have tests**
- **Test coverage must be ≥90%**
- **Safety-critical code must have 100% coverage**
- **Performance tests must validate timing requirements**

## Documentation Requirements

### Code Documentation

- **All public APIs must be documented** with Doxygen comments
- **Complex algorithms must have explanatory comments**
- **Safety-critical code must have detailed documentation**

### User Documentation

- **Update user manual** for user-facing changes
- **Update API reference** for developer-facing changes
- **Add troubleshooting entries** for known issues

## Pull Request Process

### Before Submitting

1. **Ensure all tests pass**
2. **Run static analysis tools**
3. **Update documentation**
4. **Rebase on latest main branch**
5. **Use descriptive commit messages**

### Pull Request Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update
- [ ] Safety-critical change

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests pass
- [ ] Safety tests pass (if applicable)
- [ ] Performance tests pass
- [ ] Manual testing completed

## Documentation
- [ ] Code comments updated
- [ ] API documentation updated
- [ ] User manual updated (if needed)

## Safety Impact
- [ ] No safety impact
- [ ] Safety impact assessed and documented
- [ ] Safety validation completed

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Tests added for new functionality
- [ ] Documentation updated
- [ ] No new warnings introduced
```

### Review Process

1. **Automated checks** must pass (CI/CD)
2. **Code review** by at least one maintainer
3. **Safety review** for safety-critical changes
4. **Performance validation** for performance-sensitive changes

## Branching Strategy

- **main** - Production-ready code
- **develop** - Integration branch for new features
- **feature/*** - Feature development branches
- **hotfix/*** - Critical bug fixes
- **release/*** - Release preparation branches

## Release Process

1. **Feature freeze** on develop branch
2. **Create release branch** from develop
3. **Final testing and validation**
4. **Update version numbers and changelog**
5. **Merge to main and tag release**
6. **Deploy and monitor**

## Security Considerations

### Reporting Security Issues

- **Do not create public issues** for security vulnerabilities
- **Email maintainers directly** with security concerns
- **Provide detailed information** about the vulnerability
- **Allow time for assessment** before public disclosure

### Security Guidelines

- **Validate all inputs** from users and external sources
- **Use secure coding practices** to prevent common vulnerabilities
- **Minimize attack surface** by limiting external dependencies
- **Regular security audits** of critical components

## Medical Device Compliance

### Regulatory Considerations

- **IEC 60601-1** - Medical electrical equipment safety
- **ISO 14971** - Risk management for medical devices
- **IEC 62304** - Medical device software lifecycle
- **FDA 21 CFR Part 820** - Quality system regulation

### Validation Requirements

- **Risk analysis** for all changes
- **Traceability** from requirements to implementation
- **Verification and validation** documentation
- **Change control** procedures

## Getting Help

### Communication Channels

- **GitHub Issues** - Bug reports and feature requests
- **GitHub Discussions** - General questions and discussions
- **Email** - Direct contact with maintainers

### Resources

- **User Manual** - Complete operation guide
- **API Reference** - Developer documentation
- **Architecture Documentation** - System design overview
- **Testing Guide** - How to run and write tests

## Recognition

Contributors will be recognized in:
- **CONTRIBUTORS.md** file
- **Release notes** for significant contributions
- **Project documentation** for major features

Thank you for contributing to the Vacuum Controller GUI System!
