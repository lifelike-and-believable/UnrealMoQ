# UnrealMoQ Examples

This directory contains example code demonstrating how to use the UnrealMoQ plugin.

## MoqExampleActor

A complete example Actor that demonstrates:
- Connecting to a MoQ relay server
- Publishing real-time position data
- Handling connection state changes
- Using both Blueprint and C++ patterns

### Usage

**Option 1: Copy to Your Project (Recommended)**

1. Copy `MoqExampleActor.h` to your project's `Source/YourProject/Public/` directory
2. Copy `MoqExampleActor.cpp` to your project's `Source/YourProject/Private/` directory
3. Rebuild your project
4. Place the actor in your level and configure the relay URL

**Option 2: Reference in Blueprints**

The example demonstrates patterns you can use in your Blueprint implementations:
- Event-driven connection management
- Publishing data at intervals
- Handling callbacks on the game thread

### Configuration

Edit the following properties in the Details panel:

- **Relay Url**: URL of the MoQ relay server (default: CloudFlare's public relay)
- **Publish Namespace**: Namespace for your published tracks (default: "example")
- **Publish Track Name**: Name of the track to publish (default: "actor-position")
- **Auto Connect**: Whether to connect automatically on BeginPlay (default: true)
- **Publish Interval**: How often to publish position updates in seconds (default: 0.1)

### What It Does

1. **On BeginPlay**: 
   - Creates a MoqClient instance
   - Connects to the specified relay server
   
2. **On Connection**: 
   - Announces the publishing namespace
   - Creates a publisher for position data
   
3. **During Tick**: 
   - Publishes the actor's current location as JSON
   - Uses Datagram mode for low-latency updates
   
4. **On EndPlay**: 
   - Disconnects gracefully from the relay

### Extending the Example

**Subscribe to Remote Tracks:**
```cpp
RemoteSubscriber = MoqClient->Subscribe(TEXT("remote-namespace"), TEXT("remote-track"));
if (RemoteSubscriber)
{
    RemoteSubscriber->OnDataReceived.AddDynamic(this, &AMoqExampleActor::OnDataReceived);
    RemoteSubscriber->OnTextReceived.AddDynamic(this, &AMoqExampleActor::OnTextReceived);
}
```

**Publish Binary Data:**
```cpp
TArray<uint8> BinaryData;
// ... populate BinaryData ...
LocationPublisher->PublishData(BinaryData, EMoqDeliveryMode::Stream);
```

**Handle Different Message Types:**
```cpp
void AMoqExampleActor::OnTextReceived(FString Text)
{
    // Parse JSON
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        // Extract fields
        FString MessageType = JsonObject->GetStringField(TEXT("type"));
        
        if (MessageType == TEXT("position"))
        {
            float X = JsonObject->GetNumberField(TEXT("x"));
            float Y = JsonObject->GetNumberField(TEXT("y"));
            float Z = JsonObject->GetNumberField(TEXT("z"));
            
            // Update remote actor position
            FVector RemotePosition(X, Y, Z);
            // ...
        }
    }
}
```

## Creating Your Own Examples

When creating your own implementations:

1. **Always check connection state** before publishing/subscribing
2. **Handle all connection states** in the callback
3. **Announce namespaces** before creating publishers
4. **Use Datagram mode** for real-time, high-frequency data
5. **Use Stream mode** for critical, must-arrive data
6. **Clean up on EndPlay** to disconnect gracefully

## More Examples

For more examples and patterns, see:
- [Integration Guide](../INTEGRATION_GUIDE.md)
- [README](../README.md)
- [moq-ffi examples](../External/moq-ffi/examples/)

## Support

If you have questions or need help with the examples:
- [GitHub Issues](https://github.com/lifelike-and-believable/UnrealMoQ/issues)
- [GitHub Discussions](https://github.com/lifelike-and-believable/UnrealMoQ/discussions)
