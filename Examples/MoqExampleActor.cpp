// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqExampleActor.h"
#include "MoqBlueprintLibrary.h"

AMoqExampleActor::AMoqExampleActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PublishTimeAccumulator = 0.0f;
}

void AMoqExampleActor::BeginPlay()
{
	Super::BeginPlay();

	// Create MoQ client
	MoqClient = NewObject<UMoqClient>(this);
	
	// Bind to connection state changes
	MoqClient->OnConnectionStateChanged.AddDynamic(this, &AMoqExampleActor::OnConnectionStateChanged);

	// Auto-connect if enabled
	if (bAutoConnect)
	{
		Connect();
	}
}

void AMoqExampleActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Disconnect();
	Super::EndPlay(EndPlayReason);
}

void AMoqExampleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Publish location at specified interval
	if (LocationPublisher && MoqClient && MoqClient->IsConnected())
	{
		PublishTimeAccumulator += DeltaTime;
		
		if (PublishTimeAccumulator >= PublishInterval)
		{
			PublishLocation();
			PublishTimeAccumulator = 0.0f;
		}
	}
}

void AMoqExampleActor::Connect()
{
	if (!MoqClient)
	{
		UE_LOG(LogTemp, Error, TEXT("MoqExampleActor: MoqClient not initialized"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Connecting to %s"), *RelayUrl);
	
	FMoqResult Result = MoqClient->Connect(RelayUrl);
	if (!Result.bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("MoqExampleActor: Failed to connect: %s"), *Result.ErrorMessage);
	}
}

void AMoqExampleActor::Disconnect()
{
	if (MoqClient && MoqClient->IsConnected())
	{
		UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Disconnecting"));
		MoqClient->Disconnect();
	}
}

void AMoqExampleActor::PublishLocation()
{
	if (!LocationPublisher)
	{
		return;
	}

	// Get current location
	FVector Location = GetActorLocation();

	// Create JSON message with location
	FString Message = FString::Printf(
		TEXT("{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"timestamp\":%lld}"),
		Location.X,
		Location.Y,
		Location.Z,
		FDateTime::UtcNow().ToUnixTimestamp()
	);

	// Publish using datagram for low-latency updates
	FMoqResult Result = LocationPublisher->PublishText(Message, EMoqDeliveryMode::Datagram);
	
	if (!Result.bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoqExampleActor: Failed to publish: %s"), *Result.ErrorMessage);
	}
}

void AMoqExampleActor::OnConnectionStateChanged(EMoqConnectionState NewState)
{
	switch (NewState)
	{
	case EMoqConnectionState::Connected:
		UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Connected to relay"));
		
		// Announce our namespace
		FMoqResult AnnounceResult = MoqClient->AnnounceNamespace(PublishNamespace);
		if (!AnnounceResult.bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("MoqExampleActor: Failed to announce namespace: %s"), 
				*AnnounceResult.ErrorMessage);
			return;
		}

		// Create publisher for location data
		LocationPublisher = MoqClient->CreatePublisher(
			PublishNamespace,
			PublishTrackName,
			EMoqDeliveryMode::Datagram
		);

		if (LocationPublisher)
		{
			UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Created publisher for %s/%s"), 
				*PublishNamespace, *PublishTrackName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MoqExampleActor: Failed to create publisher"));
		}

		// Optionally subscribe to remote track (for demo purposes)
		// In a real application, you might subscribe to a different namespace/track
		/*
		RemoteSubscriber = MoqClient->Subscribe(TEXT("remote-namespace"), TEXT("remote-track"));
		if (RemoteSubscriber)
		{
			RemoteSubscriber->OnDataReceived.AddDynamic(this, &AMoqExampleActor::OnDataReceived);
			RemoteSubscriber->OnTextReceived.AddDynamic(this, &AMoqExampleActor::OnTextReceived);
			UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Subscribed to remote track"));
		}
		*/
		break;

	case EMoqConnectionState::Disconnected:
		UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Disconnected from relay"));
		LocationPublisher = nullptr;
		RemoteSubscriber = nullptr;
		break;

	case EMoqConnectionState::Connecting:
		UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Connecting..."));
		break;

	case EMoqConnectionState::Failed:
		UE_LOG(LogTemp, Error, TEXT("MoqExampleActor: Connection failed"));
		break;
	}
}

void AMoqExampleActor::OnDataReceived(const TArray<uint8>& Data)
{
	UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Received %d bytes of data"), Data.Num());
	
	// Process binary data here
	// For example, parse custom binary protocol
}

void AMoqExampleActor::OnTextReceived(FString Text)
{
	UE_LOG(LogTemp, Log, TEXT("MoqExampleActor: Received text: %s"), *Text);
	
	// Process text data here
	// For example, parse JSON message with remote actor position
}
