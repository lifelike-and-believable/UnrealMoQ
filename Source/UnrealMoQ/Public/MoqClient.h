// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "moq_ffi.h"
#include "MoqClient.generated.h"

class UMoqPublisher;
class UMoqSubscriber;

/** Connection state enum for Blueprint */
UENUM(BlueprintType)
enum class EMoqConnectionState : uint8
{
	Disconnected = 0 UMETA(DisplayName = "Disconnected"),
	Connecting = 1 UMETA(DisplayName = "Connecting"),
	Connected = 2 UMETA(DisplayName = "Connected"),
	Failed = 3 UMETA(DisplayName = "Failed")
};

/** Delivery mode enum for Blueprint */
UENUM(BlueprintType)
enum class EMoqDeliveryMode : uint8
{
	Datagram = 0 UMETA(DisplayName = "Datagram (Lossy, Low Latency)"),
	Stream = 1 UMETA(DisplayName = "Stream (Reliable, Ordered)")
};

/** Result structure for Blueprint */
USTRUCT(BlueprintType)
struct FMoqResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "MoQ")
	bool bSuccess;

	UPROPERTY(BlueprintReadOnly, Category = "MoQ")
	FString ErrorMessage;

	FMoqResult()
		: bSuccess(false)
	{
	}

	FMoqResult(bool InSuccess, const FString& InErrorMessage = FString())
		: bSuccess(InSuccess)
		, ErrorMessage(InErrorMessage)
	{
	}
};

/** Delegate for connection state changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoqConnectionStateChanged, EMoqConnectionState, NewState);

/** Delegate for track announcements */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMoqTrackAnnounced, FString, Namespace, FString, TrackName);

/**
 * UMoqClient - Unreal wrapper for MoQ client functionality
 * 
 * This class provides a Blueprint-friendly interface to the Media over QUIC (MoQ) protocol.
 * It manages connection to MoQ relay servers, publishing and subscribing to media tracks.
 */
UCLASS(BlueprintType)
class UNREALMOQ_API UMoqClient : public UObject
{
	GENERATED_BODY()

public:
	UMoqClient();
	virtual ~UMoqClient();

	// UObject interface
	virtual void BeginDestroy() override;

	/**
	 * Connect to a MoQ relay server
	 * @param Url Connection URL (e.g., "https://relay.example.com:443")
	 * @return Result of the connection attempt
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Client")
	FMoqResult Connect(const FString& Url);

	/**
	 * Disconnect from the MoQ relay
	 * @return Result of the disconnection
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Client")
	FMoqResult Disconnect();

	/**
	 * Check if client is currently connected
	 * @return True if connected, false otherwise
	 */
	UFUNCTION(BlueprintPure, Category = "MoQ|Client")
	bool IsConnected() const;

	/**
	 * Announce a namespace for publishing
	 * @param Namespace Namespace to announce
	 * @return Result of the announcement
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Publishing")
	FMoqResult AnnounceNamespace(const FString& Namespace);

	/**
	 * Create a publisher for a specific track
	 * @param Namespace Namespace of the track
	 * @param TrackName Name of the track
	 * @param DeliveryMode Delivery mode (datagram or stream)
	 * @return Handle to the publisher or null on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Publishing")
	UMoqPublisher* CreatePublisher(const FString& Namespace, const FString& TrackName, EMoqDeliveryMode DeliveryMode = EMoqDeliveryMode::Stream);

	/**
	 * Subscribe to a track
	 * @param Namespace Namespace of the track
	 * @param TrackName Name of the track
	 * @return Handle to the subscriber or null on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Subscribing")
	UMoqSubscriber* Subscribe(const FString& Namespace, const FString& TrackName);

	/** Event fired when connection state changes */
	UPROPERTY(BlueprintAssignable, Category = "MoQ|Events")
	FMoqConnectionStateChanged OnConnectionStateChanged;

	/** Event fired when a track is announced */
	UPROPERTY(BlueprintAssignable, Category = "MoQ|Events")
	FMoqTrackAnnounced OnTrackAnnounced;

	/** Get the underlying C API client handle (for internal use) */
	MoqClient* GetClientHandle() const { return ClientHandle; }

private:
	/** Handle to the native MoQ client */
	MoqClient* ClientHandle;

	/** C callback for connection state changes */
	static void OnConnectionStateChangedCallback(void* UserData, MoqConnectionState State);

	/** C callback for track announcements */
	static void OnTrackAnnouncedCallback(void* UserData, const char* Namespace, const char* TrackName);

	/** Current connection state */
	EMoqConnectionState CurrentState;
};
