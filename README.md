# UnrealMoQ

Media over QUIC (MoQ) C++ / Blueprint Library for Unreal Engine 5.7+

## Overview

UnrealMoQ is a cross-platform Unreal Engine plugin that provides a Blueprint and C++ API for the [Media over QUIC (MoQ)](https://datatracker.ietf.org/doc/draft-ietf-moq-transport/) protocol. It wraps the [moq-ffi](https://github.com/lifelike-and-believable/moq-ffi) library to enable low-latency media streaming in Unreal Engine applications.

### Features

- ✅ **Cross-Platform**: Windows, Linux, and macOS support
- ✅ **Blueprint & C++ API**: Full Blueprint support with C++ extensibility
- ✅ **MoQ Lifecycle Management**: Connect, publish, subscribe, and disconnect
- ✅ **Dual Delivery Modes**: Datagram (lossy, low-latency) and Stream (reliable)
- ✅ **Event-Driven Architecture**: Connection state, data received, and track announcement events
- ✅ **WebTransport Support**: Connect to MoQ relays via HTTPS (WebTransport over QUIC)
- ✅ **Synchronized with moq-ffi**: Git submodule keeps plugin in sync with library updates

### Supported Platforms

| Platform | Architecture | Status |
|----------|-------------|---------|
| Windows  | x64         | ✅ Supported |
| Linux    | x64         | ✅ Supported |
| macOS    | Universal (x64 + ARM64) | ✅ Supported |

## Installation

### Prerequisites

1. **Unreal Engine 5.7+** installed
2. **Rust toolchain** (for building moq-ffi):
   - Windows: Install via [rustup](https://rustup.rs/) and ensure MSVC build tools are available
   - Linux: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`
   - macOS: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`

### Plugin Installation

1. **Clone the repository** into your Unreal project's `Plugins` directory:
   ```bash
   cd YourProject/Plugins
   git clone --recursive https://github.com/lifelike-and-believable/UnrealMoQ.git
   ```

2. **Build the moq-ffi library**:

   **Windows (PowerShell):**
   ```powershell
   cd UnrealMoQ/ThirdParty/moq-ffi/moq_ffi
   cargo build --release --features with_moq_draft07
   ```

   **Linux/macOS:**
   ```bash
   cd UnrealMoQ/ThirdParty/moq-ffi/moq_ffi
   cargo build --release --features with_moq_draft07
   ```

   > **Note**: Use `with_moq_draft07` for CloudFlare's production relay, or `with_moq` for IETF Draft 14.

3. **Regenerate project files** and build your Unreal project:
   ```bash
   # Right-click your .uproject file and select "Generate Visual Studio project files"
   # Or use UnrealBuildTool from command line
   ```

4. **Enable the plugin** in your project:
   - Open your project in Unreal Engine
   - Go to Edit → Plugins
   - Search for "UnrealMoQ"
   - Check the "Enabled" checkbox
   - Restart the editor

## Usage

### Blueprint Usage

#### Creating and Connecting a Client

1. **Create a MoQ Client**:
   - In your Blueprint, add a variable of type `MoqClient` (Object Reference)
   - Use "Construct Object from Class" node with `MoqClient` class

2. **Connect to a Relay**:
   - Call `Connect` on the client with a relay URL (e.g., `https://relay.cloudflare.mediaoverquic.com`)
   - Bind to `OnConnectionStateChanged` event to monitor connection status

3. **Announce a Namespace**:
   - Once connected, call `AnnounceNamespace` with your namespace string

#### Publishing Data

```
1. Create Publisher
   └─> Call "Create Publisher" on MoqClient
       ├─ Namespace: "my-namespace"
       ├─ Track Name: "my-track"
       └─ Delivery Mode: Stream or Datagram

2. Publish Data
   └─> Call "Publish Data" or "Publish Text" on the Publisher
       ├─ Data: Byte array or string
       └─ Delivery Mode: Stream or Datagram
```

#### Subscribing to Data

```
1. Subscribe to Track
   └─> Call "Subscribe" on MoqClient
       ├─ Namespace: "remote-namespace"
       └─ Track Name: "remote-track"

2. Receive Data
   └─> Bind to "On Data Received" or "On Text Received" event
       └─ Process incoming data
```

#### Disconnecting

```
Call "Disconnect" on MoqClient
└─> Connection state changes to Disconnected
```

### C++ Usage

#### Basic Example

```cpp
#include "MoqClient.h"
#include "MoqPublisher.h"
#include "MoqSubscriber.h"

// Create client
UMoqClient* Client = NewObject<UMoqClient>();

// Bind to connection state changes
Client->OnConnectionStateChanged.AddDynamic(this, &AMyActor::OnMoqConnectionStateChanged);

// Connect
FMoqResult Result = Client->Connect(TEXT("https://relay.cloudflare.mediaoverquic.com"));
if (Result.bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("Connecting to MoQ relay..."));
}

// Once connected, announce namespace
Client->AnnounceNamespace(TEXT("my-namespace"));

// Create publisher
UMoqPublisher* Publisher = Client->CreatePublisher(
    TEXT("my-namespace"),
    TEXT("my-track"),
    EMoqDeliveryMode::Stream
);

// Publish data
TArray<uint8> Data = { 0x01, 0x02, 0x03 };
Publisher->PublishData(Data, EMoqDeliveryMode::Stream);

// Subscribe to track
UMoqSubscriber* Subscriber = Client->Subscribe(TEXT("remote-namespace"), TEXT("remote-track"));
Subscriber->OnDataReceived.AddDynamic(this, &AMyActor::OnMoqDataReceived);

// Handle received data
void AMyActor::OnMoqDataReceived(const TArray<uint8>& Data)
{
    UE_LOG(LogTemp, Log, TEXT("Received %d bytes"), Data.Num());
}

// Disconnect
Client->Disconnect();
```

#### Connection State Handling

```cpp
void AMyActor::OnMoqConnectionStateChanged(EMoqConnectionState NewState)
{
    switch (NewState)
    {
    case EMoqConnectionState::Disconnected:
        UE_LOG(LogTemp, Log, TEXT("MoQ: Disconnected"));
        break;
    case EMoqConnectionState::Connecting:
        UE_LOG(LogTemp, Log, TEXT("MoQ: Connecting..."));
        break;
    case EMoqConnectionState::Connected:
        UE_LOG(LogTemp, Log, TEXT("MoQ: Connected!"));
        // Now safe to announce namespaces and create publishers/subscribers
        break;
    case EMoqConnectionState::Failed:
        UE_LOG(LogTemp, Error, TEXT("MoQ: Connection failed"));
        break;
    }
}
```

## API Reference

### UMoqClient

Main client class for managing MoQ connections.

**Methods:**
- `FMoqResult Connect(const FString& Url)` - Connect to a MoQ relay
- `FMoqResult Disconnect()` - Disconnect from the relay
- `bool IsConnected()` - Check connection status
- `FMoqResult AnnounceNamespace(const FString& Namespace)` - Announce a publishing namespace
- `UMoqPublisher* CreatePublisher(const FString& Namespace, const FString& TrackName, EMoqDeliveryMode DeliveryMode)` - Create a publisher
- `UMoqSubscriber* Subscribe(const FString& Namespace, const FString& TrackName)` - Subscribe to a track

**Events:**
- `OnConnectionStateChanged(EMoqConnectionState NewState)` - Connection state changes
- `OnTrackAnnounced(FString Namespace, FString TrackName)` - Remote track announced

### UMoqPublisher

Publisher for sending data on a MoQ track.

**Methods:**
- `FMoqResult PublishData(const TArray<uint8>& Data, EMoqDeliveryMode DeliveryMode)` - Publish binary data
- `FMoqResult PublishText(const FString& Text, EMoqDeliveryMode DeliveryMode)` - Publish text (UTF-8 encoded)

### UMoqSubscriber

Subscriber for receiving data from a MoQ track.

**Events:**
- `OnDataReceived(const TArray<uint8>& Data)` - Binary data received
- `OnTextReceived(FString Text)` - Text data received (UTF-8 decoded)

### UMoqBlueprintLibrary

Utility functions for MoQ operations.

**Methods:**
- `FString GetMoqVersion()` - Get moq-ffi library version
- `FString GetLastError()` - Get last error message
- `FString BytesToString(const TArray<uint8>& Data)` - Convert bytes to string (UTF-8)
- `TArray<uint8> StringToBytes(const FString& Text)` - Convert string to bytes (UTF-8)

### Enums

**EMoqConnectionState:**
- `Disconnected` - Not connected
- `Connecting` - Connection in progress
- `Connected` - Successfully connected
- `Failed` - Connection failed

**EMoqDeliveryMode:**
- `Datagram` - Lossy, low-latency delivery (best for high-frequency updates)
- `Stream` - Reliable, ordered delivery (best for critical data)

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Unreal Engine                        │
│  ┌──────────────────────────────────────────────────┐  │
│  │          Blueprint / C++ Game Code               │  │
│  └────────────────────┬─────────────────────────────┘  │
│                       │                                 │
│  ┌────────────────────▼─────────────────────────────┐  │
│  │         UnrealMoQ Plugin Layer                   │  │
│  │  • UMoqClient (UObject)                          │  │
│  │  • UMoqPublisher (UObject)                       │  │
│  │  • UMoqSubscriber (UObject)                      │  │
│  │  • Blueprint Function Library                    │  │
│  │  • Event Delegates                               │  │
│  └────────────────────┬─────────────────────────────┘  │
│                       │                                 │
│  ┌────────────────────▼─────────────────────────────┐  │
│  │         moq-ffi C API (moq_ffi.h)                │  │
│  │  • moq_client_create/destroy                     │  │
│  │  • moq_connect/disconnect                        │  │
│  │  • moq_publish_data                              │  │
│  │  • moq_subscribe                                 │  │
│  └────────────────────┬─────────────────────────────┘  │
└───────────────────────┼─────────────────────────────────┘
                        │
         ┌──────────────▼───────────────┐
         │   moq-ffi Rust FFI Library   │
         │  (moq_ffi.dll/.so/.dylib)    │
         └──────────────┬───────────────┘
                        │
         ┌──────────────▼───────────────┐
         │   moq-transport (Rust)       │
         │   • CloudFlare moq-rs        │
         │   • QUIC / WebTransport      │
         └──────────────────────────────┘
```

## Development

### Building from Source

1. Clone with submodules:
   ```bash
   git clone --recursive https://github.com/lifelike-and-believable/UnrealMoQ.git
   ```

2. Build moq-ffi (see Installation section above)

3. Open in Unreal Engine or build via UnrealBuildTool

### Updating moq-ffi

To update the moq-ffi submodule to the latest version:

```bash
cd ThirdParty/moq-ffi
git pull origin main
cd ../..
git add ThirdParty/moq-ffi
git commit -m "Update moq-ffi submodule"
```

Then rebuild the moq-ffi library as described in the Installation section.

### Testing

UnrealMoQ includes a comprehensive test suite with 80%+ code coverage.

**Running Tests:**

1. **In Unreal Engine Editor**:
   - Open `Window` → `Developer Tools` → `Session Frontend`
   - Go to the `Automation` tab
   - Filter for "UnrealMoQ" tests
   - Select and run tests

2. **Via Command Line**:
   ```bash
   UnrealEditor-Cmd "YourProject.uproject" -ExecCmds="Automation RunTests UnrealMoQ" -unattended -nopause -NullRHI -log
   ```

**Test Coverage:**
- 60 comprehensive automation tests
- Coverage across all major components (Client, Publisher, Subscriber, Blueprint Library)
- Unit tests for API contracts, error handling, and data conversion
- No network or relay server required

For detailed testing documentation, see [Source/UnrealMoQTests/README.md](Source/UnrealMoQTests/README.md).

## Troubleshooting

### Plugin fails to load

- **Ensure moq-ffi is built**: Check that `moq_ffi.dll` (Windows), `libmoq_ffi.so` (Linux), or `libmoq_ffi.dylib` (macOS) exists in `ThirdParty/moq-ffi/moq_ffi/target/release/`
- **Check Rust toolchain**: Verify Rust is installed with `rustc --version`
- **Rebuild**: Try cleaning and rebuilding the moq-ffi library

### Connection fails

- **Check URL**: Ensure the relay URL is correct and accessible
- **Protocol support**: Use `https://` URLs for WebTransport (CloudFlare relay)
- **Draft version**: Ensure you built moq-ffi with the correct feature flag (`with_moq_draft07` for CloudFlare)

### Data not received

- **Check connection state**: Ensure client is in `Connected` state before publishing/subscribing
- **Namespace announced**: Publishers must announce their namespace before publishing
- **Event binding**: Ensure event delegates are bound before data is received

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Related Projects

- [moq-ffi](https://github.com/lifelike-and-believable/moq-ffi) - C FFI wrapper for moq-rs
- [moq-rs](https://github.com/cloudflare/moq-rs) - CloudFlare's Rust implementation of MoQ Transport
- [MoQ IETF Draft](https://datatracker.ietf.org/doc/draft-ietf-moq-transport/) - MoQ Transport specification

## Support

- **Issues**: [GitHub Issues](https://github.com/lifelike-and-believable/UnrealMoQ/issues)
- **Discussions**: [GitHub Discussions](https://github.com/lifelike-and-believable/UnrealMoQ/discussions)
