// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "MoqTypes.h"
#include "moq_ffi.h"
#include "MoqBlueprintAsyncActions.generated.h"

class UMoqClient;
class UMoqSubscriber;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoqConnectAsyncSuccess, UMoqClient*, Client);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMoqConnectAsyncFailure, UMoqClient*, Client, const FString&, ErrorMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoqSubscribeAsyncSuccess, UMoqSubscriber*, Subscriber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMoqSubscribeAsyncFailure, UMoqClient*, Client, const FString&, ErrorMessage);

/**
 * Blueprint-friendly latent node that connects a UMoqClient and waits for the connected state.
 */
UCLASS()
class UNREALMOQ_API UMoqConnectClientAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Begin the connection process for the provided client. */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Blueprint", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UMoqConnectClientAsyncAction* ConnectClient(UObject* WorldContextObject, UMoqClient* Client, const FString& RelayUrl, float TimeoutSeconds = 15.0f);

	/** Called when the client reports EMoqConnectionState::Connected. */
	UPROPERTY(BlueprintAssignable)
	FMoqConnectAsyncSuccess OnConnected;

	/** Called when the connection attempt fails or times out. */
	UPROPERTY(BlueprintAssignable)
	FMoqConnectAsyncFailure OnFailed;
	/** Abort the pending connection attempt, broadcasting OnFailed immediately. */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Blueprint")
	void Cancel();

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;

private:
	void RegisterWorldCleanupListener();
	void UnregisterWorldCleanupListener();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	UWorld* ResolveWorld() const;
	void HandleConnectionStateChanged(EMoqConnectionState NewState);
	bool HandleTickerTimeout(float DeltaTime);
	void FinishSuccess();
	void FinishFailure(const FString& ErrorMessage);
	void Cleanup();

	bool bHasTriggered = false;
	bool bCancellationRequested = false;
	TWeakObjectPtr<UMoqClient> ClientPtr;
	FString TargetRelay;
	float TimeoutSeconds = 15.0f;
	double StartTimeSeconds = 0.0;
	FTSTicker::FDelegateHandle TickerHandle;
	bool bTickerRegistered = false;
	TWeakObjectPtr<UObject> WeakWorldContext;
	TWeakObjectPtr<UWorld> CachedWorld;
	FDelegateHandle WorldCleanupHandle;
};

/**
 * Blueprint latent node that retries Subscribe() calls to accommodate asynchronous track announcements.
 */
UCLASS()
class UNREALMOQ_API UMoqSubscribeWithRetryAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MoQ|Blueprint", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UMoqSubscribeWithRetryAsyncAction* SubscribeWithRetry(UObject* WorldContextObject, UMoqClient* Client, const FString& Namespace, const FString& TrackName, int32 MaxAttempts = 3, float RetryDelaySeconds = 0.5f);
	/** Cancel all future retry attempts and broadcast OnFailed immediately. */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Blueprint")
	void Cancel();

	UPROPERTY(BlueprintAssignable)
	FMoqSubscribeAsyncSuccess OnSubscribed;

	UPROPERTY(BlueprintAssignable)
	FMoqSubscribeAsyncFailure OnFailed;

	virtual void Activate() override;

private:
	void RegisterWorldCleanupListener();
	void UnregisterWorldCleanupListener();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	UWorld* ResolveWorld() const;
	void AttemptSubscribe();
	bool HandleRetryTicker(float DeltaTime);
	void FinishSuccess(UMoqSubscriber* Subscriber);
	void FinishFailure(const FString& ErrorMessage);
	FString DescribeLastError() const;
	void Cleanup();

	TWeakObjectPtr<UMoqClient> ClientPtr;
	FString NamespaceValue;
	FString TrackValue;
	int32 MaxAttempts = 3;
	float RetryDelaySeconds = 0.5f;
	int32 AttemptCounter = 0;
	double NextAttemptTime = 0.0;
	bool bHasResolved = false;
	bool bCancellationRequested = false;
	FTSTicker::FDelegateHandle RetryTickerHandle;
	TWeakObjectPtr<UObject> WeakWorldContext;
	TWeakObjectPtr<UWorld> CachedWorld;
	FDelegateHandle WorldCleanupHandle;
};
