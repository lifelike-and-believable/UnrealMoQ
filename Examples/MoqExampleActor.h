// Copyright Epic Games, Inc. All Rights Reserved.
// Example Actor demonstrating UnrealMoQ usage

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoqClient.h"
#include "MoqPublisher.h"
#include "MoqSubscriber.h"
#include "MoqExampleActor.generated.h"

/**
 * Example Actor demonstrating basic MoQ functionality
 * 
 * This actor shows how to:
 * - Connect to a MoQ relay
 * - Publish data on a track
 * - Subscribe to remote tracks
 * - Handle connection state changes
 * - Process received data
 * 
 * To use this example:
 * 1. Place this actor in your level
 * 2. Set the RelayUrl property (default: CloudFlare relay)
 * 3. Play in editor
 * 4. The actor will connect and start publishing its location
 */
UCLASS()
class AMoqExampleActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMoqExampleActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:	
	/** URL of the MoQ relay server */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoQ Example")
	FString RelayUrl = TEXT("https://relay.cloudflare.mediaoverquic.com");

	/** Namespace for publishing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoQ Example")
	FString PublishNamespace = TEXT("example");

	/** Track name for publishing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoQ Example")
	FString PublishTrackName = TEXT("actor-position");

	/** Whether to automatically connect on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoQ Example")
	bool bAutoConnect = true;

	/** How often to publish position updates (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoQ Example")
	float PublishInterval = 0.1f;

	/** Connect to the MoQ relay */
	UFUNCTION(BlueprintCallable, Category = "MoQ Example")
	void Connect();

	/** Disconnect from the MoQ relay */
	UFUNCTION(BlueprintCallable, Category = "MoQ Example")
	void Disconnect();

	/** Publish current actor location */
	UFUNCTION(BlueprintCallable, Category = "MoQ Example")
	void PublishLocation();

protected:
	/** MoQ client instance */
	UPROPERTY()
	UMoqClient* MoqClient;

	/** Publisher for location data */
	UPROPERTY()
	UMoqPublisher* LocationPublisher;

	/** Subscriber for remote locations (optional) */
	UPROPERTY()
	UMoqSubscriber* RemoteSubscriber;

	/** Time accumulator for publish interval */
	float PublishTimeAccumulator;

	/** Connection state changed callback */
	UFUNCTION()
	void OnConnectionStateChanged(EMoqConnectionState NewState);

	/** Data received callback */
	UFUNCTION()
	void OnDataReceived(const TArray<uint8>& Data);

	/** Text received callback */
	UFUNCTION()
	void OnTextReceived(FString Text);
};
