# Changelog

All notable changes to the UnrealMoQ plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - TBD

### Added
- Initial release of UnrealMoQ plugin
- Full wrapper for moq-ffi library exposing MoQ protocol to Unreal Engine
- Cross-platform support (Windows, Linux, macOS)
- Blueprint and C++ API for all MoQ lifecycle operations
- `UMoqClient` class for connection management
- `UMoqPublisher` class for publishing data on tracks
- `UMoqSubscriber` class for subscribing to tracks
- `UMoqBlueprintLibrary` for utility functions
- Event-driven architecture with Blueprint-compatible delegates
- Support for both Datagram (lossy) and Stream (reliable) delivery modes
- Automatic library initialization and cleanup
- Thread-safe callbacks marshalled to game thread
- moq-ffi as git submodule for synchronized updates
- Comprehensive documentation and integration guide
- Connection state monitoring
- Binary and text data publishing/receiving
- Namespace announcement support

### Supported Platforms
- Windows x64
- Linux x64
- macOS Universal (x64 + ARM64)

### Dependencies
- Unreal Engine 5.7+
- moq-ffi library (included as submodule)
- Rust 1.87.0+ (for building moq-ffi)

[1.0.0]: https://github.com/lifelike-and-believable/UnrealMoQ/releases/tag/v1.0.0
