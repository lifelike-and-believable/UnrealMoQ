// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MoqClient.h"
#include "moq_ffi.h"
#include "MoqSubscriber.generated.h"

/** Delegate for data received events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoqDataReceived, const TArray<uint8>&, Data);

/** Delegate for text received events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoqTextReceived, FString, Text);

/**
 * UMoqSubscriber - Unreal wrapper for MoQ subscriber functionality
 * 
 * This class provides a Blueprint-friendly interface to subscribe to data on a MoQ track.
 */
UCLASS(BlueprintType)
class UNREALMOQ_API UMoqSubscriber : public UObject
{
	GENERATED_BODY()

public:
	UMoqSubscriber();
	virtual ~UMoqSubscriber();

	// UObject interface
	virtual void BeginDestroy() override;

	/** Event fired when binary data is received */
	UPROPERTY(BlueprintAssignable, Category = "MoQ|Events")
	FMoqDataReceived OnDataReceived;

	/** Event fired when text data is received (convenience, attempts UTF-8 decode) */
	UPROPERTY(BlueprintAssignable, Category = "MoQ|Events")
	FMoqTextReceived OnTextReceived;

	/** Initialize from native handle (internal use) */
	void InitializeFromHandle(MoqSubscriber* Handle);

	/** C callback for data received */
	static void OnDataReceivedCallback(void* UserData, const uint8_t* Data, size_t DataLen);

private:
	/** Handle to the native MoQ subscriber */
	MoqSubscriber* SubscriberHandle;
};
