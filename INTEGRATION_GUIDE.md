# UnrealMoQ Integration Guide

This guide provides step-by-step instructions for integrating the UnrealMoQ plugin into your Unreal Engine project.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building moq-ffi](#building-moq-ffi)
3. [Plugin Integration](#plugin-integration)
4. [Configuration](#configuration)
5. [Blueprint Examples](#blueprint-examples)
6. [C++ Examples](#c-examples)
7. [Packaging Your Game](#packaging-your-game)
8. [Platform-Specific Notes](#platform-specific-notes)

## Prerequisites

### Required Software

| Software | Version | Purpose |
|----------|---------|---------|
| Unreal Engine | 5.7+ | Game engine |
| Rust | 1.87.0+ | Building moq-ffi library |
| Visual Studio | 2019+ | Windows C++ compilation |
| Xcode | Latest | macOS C++ compilation |
| GCC/Clang | 7+/10+ | Linux C++ compilation |

### Installing Rust

**Windows:**
1. Download and run [rustup-init.exe](https://rustup.rs/)
2. Follow the installation prompts
3. Ensure you have Visual Studio with C++ build tools installed
4. Open "x64 Native Tools Command Prompt for VS" for building

**Linux:**
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
```

**macOS:**
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
# Ensure Xcode command line tools are installed
xcode-select --install
```

## Building moq-ffi

The moq-ffi library must be built before using the plugin. Choose the appropriate draft version:

### Draft 07 (CloudFlare Production Relay - Recommended)

**Windows (PowerShell):**
```powershell
cd Plugins/UnrealMoQ/External/moq-ffi/moq_ffi
cargo build --release --features with_moq_draft07
pwsh ../../scripts/build-moqffi.ps1
```

**Linux/macOS:**
```bash
cd Plugins/UnrealMoQ/External/moq-ffi/moq_ffi
cargo build --release --features with_moq_draft07
pwsh ../../scripts/build-moqffi.ps1
```

### Draft 14 (Latest IETF Specification)

**Windows (PowerShell):**
```powershell
cd Plugins/UnrealMoQ/External/moq-ffi/moq_ffi
cargo build --release --features with_moq
pwsh ../../scripts/build-moqffi.ps1
```

**Linux/macOS:**
```bash
cd Plugins/UnrealMoQ/External/moq-ffi/moq_ffi
cargo build --release --features with_moq
pwsh ../../scripts/build-moqffi.ps1
```

### Verifying the Build

After building, verify that the library files exist:

**Windows:**
- `ThirdParty/moq-ffi/lib/Win64/moq_ffi.lib`

**Linux:**
- `ThirdParty/moq-ffi/lib/Linux/libmoq_ffi.a`

**macOS:**
- `ThirdParty/moq-ffi/lib/Mac/libmoq_ffi.a`

## Plugin Integration

### Option 1: Clone into Plugins Directory

```bash
cd YourProject/Plugins
git clone --recursive https://github.com/lifelike-and-believable/UnrealMoQ.git
```

### Option 2: Add as Git Submodule

```bash
cd YourProject
git submodule add https://github.com/lifelike-and-believable/UnrealMoQ.git Plugins/UnrealMoQ
git submodule update --init --recursive
```

### Enable the Plugin

1. Open your project in Unreal Engine
2. Go to **Edit → Plugins**
3. Search for "UnrealMoQ"
4. Check the **Enabled** checkbox
5. Restart the editor when prompted

### Regenerate Project Files

**Windows:**
Right-click your `.uproject` file and select "Generate Visual Studio project files"

**Linux/macOS:**
```bash
# Path to your Unreal Engine installation
/path/to/UnrealEngine/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh -project="/path/to/YourProject.uproject" -game
```

## Configuration

### Plugin Dependencies (C++)

If you're using UnrealMoQ from C++, add it to your module's dependencies:

**YourModule.Build.cs:**
```csharp
PublicDependencyModuleNames.AddRange(new string[] 
{
    "Core",
    "CoreUObject",
    "Engine",
    "UnrealMoQ"  // Add this line
});
```

### Including Headers

```cpp
#include "MoqClient.h"
#include "MoqPublisher.h"
#include "MoqSubscriber.h"
#include "MoqBlueprintLibrary.h"
```

## Blueprint Examples

### Example 1: Simple Chat Application

**Step 1: Create a Blueprint Actor**
1. Create a new Blueprint Class based on Actor
2. Name it "BP_MoqChat"

**Step 2: Add Variables**
- `MoqClient` (Object Reference) - The MoQ client instance
- `Publisher` (MoqPublisher Object Reference) - For sending messages
- `Subscriber` (MoqSubscriber Object Reference) - For receiving messages

**Step 3: Setup Connection (BeginPlay)**
```
BeginPlay
└─> Create MoQ Client (Blueprint Node)
    └─> Store in MoqClient variable
    └─> Bind Event to On Connection State Changed
    └─> Connect (URL: "https://relay.cloudflare.mediaoverquic.com")
```

**Step 4: Handle Connection State**
```
On Connection State Changed
└─> Switch on Enum (New State)
    ├─> Connected:
    │   └─> Announce Namespace ("chat")
    │   └─> Create Publisher ("chat", "messages", Stream)
    │   │   └─> Store in Publisher variable
    │   └─> Subscribe ("chat", "messages")
    │       └─> Store in Subscriber variable
    │       └─> Bind Event to On Text Received
    └─> Failed:
        └─> Print String ("Connection failed!")
```

**Step 5: Send Message (Custom Event)**
```
Custom Event: SendMessage (String: Message)
└─> Is Valid? (Publisher)
    └─> Publish Text (Message, Stream)
```

**Step 6: Receive Message**
```
On Text Received (String: Text)
└─> Print String (Text)
└─> Add to chat display UI
```

### Example 2: Real-Time Position Tracking

**Step 1: Setup Publisher**
```
BeginPlay
└─> Connect to MoQ relay
└─> Once Connected:
    └─> Announce Namespace ("tracking")
    └─> Create Publisher ("tracking", "position", Datagram)
```

**Step 2: Publish Position (Tick)**
```
Event Tick
└─> Get Actor Location
└─> Convert to JSON/Binary
└─> String to Bytes
└─> Publish Data (Data, Datagram)
```

**Step 3: Subscribe to Remote Position**
```
Once Connected:
└─> Subscribe ("tracking", "remote-position")
└─> On Data Received:
    └─> Bytes to String
    └─> Parse JSON
    └─> Update remote actor position
```

### Blueprint Automation & Testing Nodes

To simplify integration tests and UI-driven workflows, UnrealMoQ exposes async Blueprint nodes that exercise the same code paths as user widgets:

- **Connect MoQ Client (Async)** – wraps `UMoqConnectClientAsyncAction`. Provide an existing `UMoqClient`, relay URL, and optional timeout. The node raises `On Connected` when `EMoqConnectionState::Connected` fires and `On Failed` for immediate errors or timeouts.
- **Subscribe With Retry (Async)** – wraps `UMoqSubscribeWithRetryAsyncAction`. It repeatedly calls `Subscribe()` until a subscriber is returned or the retry budget is exhausted. The success delegate returns the ready `UMoqSubscriber` so you can immediately bind to `OnDataReceived`/`OnTextReceived`.
- **Pump MoQ Event Loop** – `UMoqAutomationBlueprintLibrary::PumpMoqEventLoop` is a lightweight helper that Blueprint functional tests or UI harnesses can call to tick the core ticker/task graph while waiting for async nodes to resolve (useful inside `FunctionalTest` Blueprints or automation macros).

These helpers are used by the new automation test `UnrealMoQ.Network.CloudflareBlueprintPublishSubscribe`, which mirrors the existing C++ Cloudflare test but drives the workflow entirely through Blueprint-accessible nodes. Enable it with the same environment variables as the other network tests:

```powershell
$env:MOQ_AUTOMATION_ENABLE_NETWORK=1
UEEditor-Cmd.exe <Project> -run=Automation RunTests "UnrealMoQ.Network.CloudflareBlueprintPublishSubscribe"
```

When authoring Blueprint-based integration tests:

1. Use **Connect MoQ Client (Async)** during `BeginPlay` (or UI init) and wait for the `On Connected` pin before announcing namespaces or creating publishers.
2. Prime tracks (publish a sentinel payload) before invoking **Subscribe With Retry**; Blueprint automation can call `Pump MoQ Event Loop` in a loop while waiting for the subscriber-ready boolean to flip.
3. Bind `OnDataReceived`/`OnTextReceived` immediately in the async success callback, ensuring test harnesses capture the payloads without relying on latent nodes.

## C++ Examples

### Example 1: Creating a MoQ Client Manager

**MoqClientManager.h:**
```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoqClient.h"
#include "MoqClientManager.generated.h"

UCLASS()
class YOURPROJECT_API AMoqClientManager : public AActor
{
    GENERATED_BODY()
    
public:    
    AMoqClientManager();
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    void ConnectToRelay(const FString& RelayUrl);
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    void DisconnectFromRelay();
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    UMoqPublisher* GetOrCreatePublisher(const FString& Namespace, const FString& TrackName);
    
protected:
    UPROPERTY()
    UMoqClient* MoqClient;
    
    UPROPERTY()
    TMap<FString, UMoqPublisher*> Publishers;
    
    UFUNCTION()
    void OnMoqConnectionStateChanged(EMoqConnectionState NewState);
};
```

**MoqClientManager.cpp:**
```cpp
#include "MoqClientManager.h"

AMoqClientManager::AMoqClientManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMoqClientManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Create MoQ client
    MoqClient = NewObject<UMoqClient>(this);
    MoqClient->OnConnectionStateChanged.AddDynamic(this, &AMoqClientManager::OnMoqConnectionStateChanged);
}

void AMoqClientManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (MoqClient && MoqClient->IsConnected())
    {
        MoqClient->Disconnect();
    }
    
    Super::EndPlay(EndPlayReason);
}

void AMoqClientManager::ConnectToRelay(const FString& RelayUrl)
{
    if (!MoqClient)
    {
        UE_LOG(LogTemp, Error, TEXT("MoqClient is not initialized"));
        return;
    }
    
    FMoqResult Result = MoqClient->Connect(RelayUrl);
    if (Result.bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Connecting to MoQ relay: %s"), *RelayUrl);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect: %s"), *Result.ErrorMessage);
    }
}

void AMoqClientManager::DisconnectFromRelay()
{
    if (MoqClient)
    {
        MoqClient->Disconnect();
    }
}

UMoqPublisher* AMoqClientManager::GetOrCreatePublisher(const FString& Namespace, const FString& TrackName)
{
    FString Key = FString::Printf(TEXT("%s/%s"), *Namespace, *TrackName);
    
    if (Publishers.Contains(Key))
    {
        return Publishers[Key];
    }
    
    if (!MoqClient || !MoqClient->IsConnected())
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create publisher: Not connected"));
        return nullptr;
    }
    
    // Announce namespace first
    MoqClient->AnnounceNamespace(Namespace);
    
    // Create publisher
    UMoqPublisher* Publisher = MoqClient->CreatePublisher(Namespace, TrackName, EMoqDeliveryMode::Stream);
    if (Publisher)
    {
        Publishers.Add(Key, Publisher);
    }
    
    return Publisher;
}

void AMoqClientManager::OnMoqConnectionStateChanged(EMoqConnectionState NewState)
{
    switch (NewState)
    {
    case EMoqConnectionState::Connected:
        UE_LOG(LogTemp, Log, TEXT("MoQ: Connected to relay"));
        break;
    case EMoqConnectionState::Disconnected:
        UE_LOG(LogTemp, Log, TEXT("MoQ: Disconnected from relay"));
        Publishers.Empty();
        break;
    case EMoqConnectionState::Failed:
        UE_LOG(LogTemp, Error, TEXT("MoQ: Connection failed"));
        break;
    default:
        break;
    }
}
```

### Example 2: Publishing Game Events

```cpp
void AMyGameMode::PublishGameEvent(const FString& EventType, const FString& EventData)
{
    if (!MoqPublisher)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoQ publisher not available"));
        return;
    }
    
    // Create JSON payload
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("type"), EventType);
    JsonObject->SetStringField(TEXT("data"), EventData);
    JsonObject->SetNumberField(TEXT("timestamp"), FDateTime::UtcNow().ToUnixTimestamp());
    
    // Serialize to string
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    // Publish
    FMoqResult Result = MoqPublisher->PublishText(JsonString, EMoqDeliveryMode::Stream);
    if (!Result.bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to publish event: %s"), *Result.ErrorMessage);
    }
}
```

## Packaging Your Game

### Windows

The DLL is automatically staged during packaging. No additional configuration needed.

### Linux

Ensure `libmoq_ffi.so` is in the correct location:
```bash
# The Build.cs handles staging automatically
# Verify in packaged game: YourGame/Binaries/Linux/libmoq_ffi.so
```

### macOS

The dylib is automatically staged. For distribution:
```bash
# Sign the dylib (if required)
codesign --force --sign "Your Identity" YourGame.app/Contents/MacOS/libmoq_ffi.dylib
```

## Platform-Specific Notes

### Windows

- **Build Tools**: Ensure Visual Studio 2019+ with C++ tools is installed
- **Rust MSVC Target**: Use the MSVC toolchain (default on Windows)
- **Build Command Prompt**: Build from "x64 Native Tools Command Prompt for VS"

### Linux

- **Dependencies**: Install build essentials: `sudo apt install build-essential pkg-config libssl-dev`
- **GLIBC Version**: Ensure target systems have compatible GLIBC version
- **Library Path**: The `.so` file is automatically staged by the build system

### macOS

- **Universal Binaries**: moq-ffi can build universal binaries (x64 + ARM64)
- **Code Signing**: May require signing for distribution
- **Xcode Tools**: Install with `xcode-select --install`

### Cross-Compilation

For cross-platform development:

1. **Build on Each Platform**: The Rust library must be built natively on each target platform
2. **Version Control**: Don't commit built binaries; each developer builds locally
3. **CI/CD**: Set up platform-specific build pipelines for automated builds

## Troubleshooting

### "Failed to load moq_ffi library"

**Cause**: The native library wasn't built or isn't in the expected location.

**Solution**:
1. Verify the platform-specific library file exists in `ThirdParty/moq-ffi/lib/<Platform>/`
2. Rebuild with: `cargo build --release --features with_moq_draft07`
3. Check build errors in the Cargo output

### "Connection failed" or "Timeout"

**Cause**: Relay server is unreachable or wrong protocol version.

**Solution**:
1. Verify relay URL is correct
2. Check network connectivity
3. Ensure correct draft version (Draft 07 for CloudFlare)
4. Check firewall settings

### "Cannot publish: Not connected"

**Cause**: Trying to publish before connection is established.

**Solution**:
1. Wait for `OnConnectionStateChanged` with `Connected` state
2. Call `AnnounceNamespace` before creating publishers
3. Check connection status with `IsConnected()`

### Build Errors in Unreal

**Cause**: Plugin module not properly configured.

**Solution**:
1. Regenerate project files
2. Clean and rebuild project
3. Verify `UnrealMoQ` is in `PublicDependencyModuleNames` in your Build.cs
4. Check that the plugin is enabled in the Plugins window

## Best Practices

1. **Connection Management**:
   - Create one `UMoqClient` per relay connection
   - Store client reference in a persistent object (e.g., GameInstance)
   - Disconnect gracefully in `EndPlay` or `BeginDestroy`

2. **Publisher/Subscriber Lifecycle**:
   - Create publishers after successful connection
   - Announce namespaces before creating publishers
   - Clean up references when disconnecting

3. **Delivery Mode Selection**:
   - Use **Datagram** for high-frequency, real-time data (position, voice)
   - Use **Stream** for critical, ordered data (chat, events)

4. **Thread Safety**:
   - All callbacks are marshalled to the game thread
   - Safe to update UI and game state in event handlers

5. **Error Handling**:
   - Always check `FMoqResult.bSuccess` after operations
   - Log error messages for debugging
   - Handle connection failures gracefully

## Next Steps

- Explore the [README](README.md) for API reference
- Check out the [moq-ffi documentation](https://github.com/lifelike-and-believable/moq-ffi)
- Review [MoQ specification](https://datatracker.ietf.org/doc/draft-ietf-moq-transport/)
- Join the community discussions

## Support

For issues and questions:
- GitHub Issues: https://github.com/lifelike-and-believable/UnrealMoQ/issues
- GitHub Discussions: https://github.com/lifelike-and-believable/UnrealMoQ/discussions
