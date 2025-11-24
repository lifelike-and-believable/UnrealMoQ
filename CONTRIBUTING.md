# Contributing to UnrealMoQ

Thank you for your interest in contributing to UnrealMoQ! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment for all contributors.

## How to Contribute

### Reporting Bugs

1. **Check existing issues** to avoid duplicates
2. **Create a new issue** with a clear title and description
3. **Include reproduction steps** and environment details:
   - Unreal Engine version
   - Platform (Windows/Linux/macOS)
   - Plugin version
   - moq-ffi version (from submodule)

### Suggesting Features

1. **Check existing issues** and discussions
2. **Create a feature request** with:
   - Clear description of the feature
   - Use cases and benefits
   - Possible implementation approach (if known)

### Submitting Changes

1. **Fork the repository**
2. **Create a feature branch** from `main`:
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. **Make your changes** following our coding standards
4. **Test your changes** thoroughly
5. **Commit with clear messages**:
   ```bash
   git commit -m "Add feature: brief description"
   ```
6. **Update documentation** if needed
7. **Submit a pull request**

## Development Setup

### Prerequisites

- Unreal Engine 5.7+
- Rust 1.87.0+ with cargo
- Git with submodule support
- Platform-specific build tools:
  - Windows: Visual Studio 2019+ with C++ tools
  - Linux: GCC 7+ or Clang 10+
  - macOS: Xcode command line tools

### Initial Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/lifelike-and-believable/UnrealMoQ.git
cd UnrealMoQ

# Build moq-ffi
cd ThirdParty/moq-ffi/moq_ffi
cargo build --release --features with_moq_draft07
cd ../../..
```

### Building and Testing

```bash
# Regenerate Unreal project files
# (Right-click .uproject or use UnrealBuildTool)

# Build in Unreal Editor
# File -> Refresh Visual Studio Project
# Build -> Build Solution

# Test in Editor
# Enable plugin in Edit -> Plugins
# Create test level with example actor
```

## Coding Standards

### C++ Style

- Follow Unreal Engine coding standards
- Use `UPROPERTY()` for Blueprint-exposed properties
- Use `UFUNCTION()` for Blueprint-exposed functions
- Use `UCLASS()` macros for Unreal reflection
- Include copyright headers in all files
- Use Unreal types: `FString`, `TArray`, etc.

### Naming Conventions

- **Classes**: PascalCase with U/A/F prefix (UMoqClient, AMoqActor)
- **Functions**: PascalCase (ConnectToRelay, PublishData)
- **Variables**: camelCase (publishInterval, moqClient)
- **Constants**: SCREAMING_SNAKE_CASE or PascalCase for static const
- **Private members**: camelCase without prefix

### File Organization

```
Source/UnrealMoQ/
├── Public/          # Public headers
│   ├── MoqClient.h
│   └── ...
├── Private/         # Implementation files
│   ├── MoqClient.cpp
│   └── ...
└── UnrealMoQ.Build.cs
```

### Comments and Documentation

- Use `/** */` for public API documentation
- Include Blueprint metadata in `UFUNCTION` declarations
- Document non-obvious implementation details
- Keep comments up to date with code changes

Example:
```cpp
/**
 * Connect to a MoQ relay server
 * @param Url Connection URL (e.g., "https://relay.example.com:443")
 * @return Result of the connection attempt
 */
UFUNCTION(BlueprintCallable, Category = "MoQ|Client")
FMoqResult Connect(const FString& Url);
```

## Testing Guidelines

### Manual Testing

1. Test on all supported platforms (Windows, Linux, macOS)
2. Test both Blueprint and C++ workflows
3. Test connection to real relay servers
4. Test publish and subscribe functionality
5. Test error handling and edge cases

### Test Scenarios

- [ ] Plugin loads successfully
- [ ] moq-ffi library initializes
- [ ] Connection to relay succeeds
- [ ] Connection failure is handled gracefully
- [ ] Namespace announcement works
- [ ] Publisher creates successfully
- [ ] Data publishes correctly
- [ ] Subscriber receives data
- [ ] Events fire on game thread
- [ ] Disconnection cleans up properly
- [ ] Multiple clients can coexist

## Pull Request Process

1. **Update documentation** if you've changed APIs
2. **Update CHANGELOG.md** with your changes
3. **Ensure all tests pass** (if applicable)
4. **Request review** from maintainers
5. **Address review feedback** promptly
6. **Squash commits** if requested

### PR Description Template

```markdown
## Description
Brief description of changes

## Motivation
Why is this change needed?

## Changes
- Change 1
- Change 2

## Testing
How was this tested?

## Checklist
- [ ] Code follows style guidelines
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Tested on Windows/Linux/macOS (mark applicable)
- [ ] No breaking changes (or documented if yes)
```

## Updating moq-ffi Submodule

When updating the moq-ffi dependency:

1. **Test with new version** locally
2. **Update submodule**:
   ```bash
   cd ThirdParty/moq-ffi
   git pull origin main
   cd ../..
   git add ThirdParty/moq-ffi
   ```
3. **Rebuild library** for all platforms
4. **Test plugin** with new version
5. **Update documentation** if API changed
6. **Create PR** with submodule update

## Documentation

### Required Documentation Updates

- **README.md**: User-facing API changes
- **INTEGRATION_GUIDE.md**: Integration process changes
- **CHANGELOG.md**: All notable changes
- **Code comments**: Implementation details

### Documentation Style

- Use clear, concise language
- Include code examples where helpful
- Link to related documentation
- Keep examples up to date

## Release Process

Maintainers follow this process for releases:

1. Update version in `UnrealMoQ.uplugin`
2. Update `CHANGELOG.md` with release notes
3. Create git tag: `git tag v1.0.0`
4. Push tag: `git push origin v1.0.0`
5. Create GitHub release with notes
6. Build release artifacts for all platforms

## Getting Help

- **Questions**: [GitHub Discussions](https://github.com/lifelike-and-believable/UnrealMoQ/discussions)
- **Bugs**: [GitHub Issues](https://github.com/lifelike-and-believable/UnrealMoQ/issues)
- **Security**: Email maintainers directly (see README)

## License

By contributing, you agree that your contributions will be licensed under the same MIT License that covers the project.

## Recognition

Contributors will be recognized in:
- CHANGELOG.md for their contributions
- GitHub contributors page
- Release notes (for significant contributions)

Thank you for contributing to UnrealMoQ! 🎮
