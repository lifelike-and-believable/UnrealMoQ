// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MoqClient.h"
#include "moq_ffi.h"
#include "MoqPublisher.generated.h"

/**
 * UMoqPublisher - Unreal wrapper for MoQ publisher functionality
 * 
 * This class provides a Blueprint-friendly interface to publish data on a MoQ track.
 */
UCLASS(BlueprintType)
class UNREALMOQ_API UMoqPublisher : public UObject
{
	GENERATED_BODY()

public:
	UMoqPublisher();
	virtual ~UMoqPublisher();

	// UObject interface
	virtual void BeginDestroy() override;

	/**
	 * Publish binary data on the track
	 * @param Data Array of bytes to publish
	 * @param DeliveryMode Delivery mode (datagram or stream)
	 * @return Result of the publish operation
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Publishing")
	FMoqResult PublishData(const TArray<uint8>& Data, EMoqDeliveryMode DeliveryMode = EMoqDeliveryMode::Stream);

	/**
	 * Publish a string as UTF-8 data on the track
	 * @param Text Text to publish
	 * @param DeliveryMode Delivery mode (datagram or stream)
	 * @return Result of the publish operation
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Publishing")
	FMoqResult PublishText(const FString& Text, EMoqDeliveryMode DeliveryMode = EMoqDeliveryMode::Stream);

	/** Initialize from native handle (internal use) */
	void InitializeFromHandle(MoqPublisher* Handle);

private:
	/** Handle to the native MoQ publisher */
	MoqPublisher* PublisherHandle;
};
