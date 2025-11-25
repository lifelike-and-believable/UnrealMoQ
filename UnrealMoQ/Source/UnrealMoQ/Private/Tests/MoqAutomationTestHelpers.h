// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MoqTypes.h"
#include "MoqBlueprintLibrary.h"

class UMoqClient;
class UMoqSubscriber;
#include "MoqAutomationTestHelpers.generated.h"

struct FMoqNetworkTestState;

/**
 * Helper UObject used to bridge dynamic multicast delegates into the automation test state.
 */
UCLASS()
class UMoqAutomationEventSink : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const TSharedPtr<FMoqNetworkTestState>& InState, bool bInIsPublisherHandler);

	UFUNCTION()
	void HandleConnectionStateChanged(EMoqConnectionState NewState);

	UFUNCTION()
	void HandleSubscriberText(FString Text);

	UFUNCTION()
	void HandleSubscriberData(const TArray<uint8>& Data);

	UFUNCTION()
	void HandleConnectAsyncSuccess(UMoqClient* Client);

	UFUNCTION()
	void HandleConnectAsyncFailure(UMoqClient* Client, const FString& ErrorMessage);

	UFUNCTION()
	void HandleSubscribeAsyncSuccess(UMoqSubscriber* Subscriber);

	UFUNCTION()
	void HandleSubscribeAsyncFailure(UMoqClient* Client, const FString& ErrorMessage);

private:
	TWeakPtr<FMoqNetworkTestState> WeakState;
	bool bIsPublisherHandler = false;
};
