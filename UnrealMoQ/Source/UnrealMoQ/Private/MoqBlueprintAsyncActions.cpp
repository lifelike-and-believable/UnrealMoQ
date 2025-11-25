// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqBlueprintAsyncActions.h"

#include "CoreMinimal.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "MoqClient.h"
#include "MoqSubscriber.h"
#include "moq_ffi.h"

UMoqConnectClientAsyncAction* UMoqConnectClientAsyncAction::ConnectClient(UObject* WorldContextObject, UMoqClient* Client, const FString& RelayUrl, float InTimeoutSeconds)
{
	UMoqConnectClientAsyncAction* Action = NewObject<UMoqConnectClientAsyncAction>();
	Action->ClientPtr = Client;
	Action->TargetRelay = RelayUrl;
	Action->TimeoutSeconds = InTimeoutSeconds > 0.f ? InTimeoutSeconds : 15.f;

	if (WorldContextObject)
	{
		Action->WeakWorldContext = WorldContextObject;
		if (UWorld* ContextWorld = WorldContextObject->GetWorld())
		{
			Action->CachedWorld = ContextWorld;
		}
		else if (GEngine)
		{
			UWorld* DerivedWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
			if (DerivedWorld)
			{
				Action->CachedWorld = DerivedWorld;
			}
		}
		Action->RegisterWithGameInstance(WorldContextObject);
	}

	return Action;
}

void UMoqConnectClientAsyncAction::Cancel()
{
	if (bHasTriggered || bCancellationRequested)
	{
		return;
	}

	bCancellationRequested = true;
	FinishFailure(TEXT("Connect request canceled"));
}

void UMoqConnectClientAsyncAction::Activate()
{
	if (bHasTriggered)
	{
		return;
	}

	if (!ClientPtr.IsValid())
	{
		FinishFailure(TEXT("Invalid MoQ client supplied to ConnectClient"));
		return;
	}

	if (TargetRelay.IsEmpty())
	{
		FinishFailure(TEXT("ConnectClient requires a non-empty relay URL"));
		return;
	}

	RegisterWorldCleanupListener();

	StartTimeSeconds = FPlatformTime::Seconds();
	ClientPtr->OnConnectionStateChanged.AddDynamic(this, &UMoqConnectClientAsyncAction::HandleConnectionStateChanged);

	const FMoqResult ConnectResult = ClientPtr->Connect(TargetRelay);
	if (!ConnectResult.bSuccess)
	{
		FinishFailure(ConnectResult.ErrorMessage.IsEmpty() ? TEXT("moq_connect returned an unspecified error") : ConnectResult.ErrorMessage);
		return;
	}

	if (!bTickerRegistered)
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UMoqConnectClientAsyncAction::HandleTickerTimeout));
		bTickerRegistered = TickerHandle.IsValid();
	}
}

void UMoqConnectClientAsyncAction::HandleConnectionStateChanged(EMoqConnectionState NewState)
{
	if (bHasTriggered)
	{
		return;
	}

	if (NewState == EMoqConnectionState::Connected)
	{
		FinishSuccess();
	}
	else if (NewState == EMoqConnectionState::Failed)
	{
		FinishFailure(TEXT("MoQ client entered Failed state while connecting"));
	}
}

bool UMoqConnectClientAsyncAction::HandleTickerTimeout(float)
{
	if (bHasTriggered)
	{
		return false;
	}

	const double Now = FPlatformTime::Seconds();
	if ((Now - StartTimeSeconds) >= TimeoutSeconds)
	{
		FinishFailure(TEXT("Timed out waiting for MoQ client to connect"));
		return false;
	}

	return true;
}

void UMoqConnectClientAsyncAction::FinishSuccess()
{
	if (bHasTriggered)
	{
		return;
	}

	bHasTriggered = true;
	OnConnected.Broadcast(ClientPtr.Get());
	Cleanup();
}

void UMoqConnectClientAsyncAction::FinishFailure(const FString& ErrorMessage)
{
	if (bHasTriggered)
	{
		return;
	}

	bHasTriggered = true;
	OnFailed.Broadcast(ClientPtr.Get(), ErrorMessage);
	Cleanup();
}

void UMoqConnectClientAsyncAction::Cleanup()
{
	if (ClientPtr.IsValid())
	{
		ClientPtr->OnConnectionStateChanged.RemoveDynamic(this, &UMoqConnectClientAsyncAction::HandleConnectionStateChanged);
	}

	if (bTickerRegistered)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle = FTSTicker::FDelegateHandle();
		bTickerRegistered = false;
	}

	UnregisterWorldCleanupListener();
	bCancellationRequested = false;

	SetReadyToDestroy();
}

void UMoqConnectClientAsyncAction::RegisterWorldCleanupListener()
{
	if (WorldCleanupHandle.IsValid())
	{
		return;
	}

	if (ResolveWorld())
	{
		WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UMoqConnectClientAsyncAction::HandleWorldCleanup);
	}
}

void UMoqConnectClientAsyncAction::UnregisterWorldCleanupListener()
{
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}
}

void UMoqConnectClientAsyncAction::HandleWorldCleanup(UWorld* World, bool, bool)
{
	if (!CachedWorld.IsValid() || CachedWorld.Get() != World)
	{
		return;
	}

	if (!bHasTriggered)
	{
		FinishFailure(TEXT("World cleaned up before connection completed"));
	}
}

UWorld* UMoqConnectClientAsyncAction::ResolveWorld() const
{
	if (CachedWorld.IsValid())
	{
		return CachedWorld.Get();
	}

	UObject* Context = WeakWorldContext.Get();
	if (!Context)
	{
		return nullptr;
	}

	UWorld* World = Context->GetWorld();
	if (!World && GEngine)
	{
		World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull);
	}

	if (World)
	{
		const_cast<UMoqConnectClientAsyncAction*>(this)->CachedWorld = World;
	}

	return World;
}

UMoqSubscribeWithRetryAsyncAction* UMoqSubscribeWithRetryAsyncAction::SubscribeWithRetry(UObject* WorldContextObject, UMoqClient* Client, const FString& Namespace, const FString& TrackName, int32 InMaxAttempts, float InRetryDelaySeconds)
{
	UMoqSubscribeWithRetryAsyncAction* Action = NewObject<UMoqSubscribeWithRetryAsyncAction>();
	Action->ClientPtr = Client;
	Action->NamespaceValue = Namespace;
	Action->TrackValue = TrackName;
	Action->MaxAttempts = FMath::Max(1, InMaxAttempts);
	Action->RetryDelaySeconds = FMath::Max(0.01f, InRetryDelaySeconds);

	if (WorldContextObject)
	{
		Action->WeakWorldContext = WorldContextObject;
		if (UWorld* ContextWorld = WorldContextObject->GetWorld())
		{
			Action->CachedWorld = ContextWorld;
		}
		else if (GEngine)
		{
			UWorld* DerivedWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
			if (DerivedWorld)
			{
				Action->CachedWorld = DerivedWorld;
			}
		}
		Action->RegisterWithGameInstance(WorldContextObject);
	}

	return Action;
}

void UMoqSubscribeWithRetryAsyncAction::Cancel()
{
	if (bHasResolved || bCancellationRequested)
	{
		return;
	}

	bCancellationRequested = true;
	FinishFailure(TEXT("Subscribe request canceled"));
}

void UMoqSubscribeWithRetryAsyncAction::Activate()
{
	if (bHasResolved)
	{
		return;
	}

	if (!ClientPtr.IsValid())
	{
		FinishFailure(TEXT("Invalid MoQ client supplied to SubscribeWithRetry"));
		return;
	}

	if (NamespaceValue.IsEmpty() || TrackValue.IsEmpty())
	{
		FinishFailure(TEXT("SubscribeWithRetry requires both Namespace and TrackName"));
		return;
	}

	RegisterWorldCleanupListener();
	AttemptSubscribe();
}

void UMoqSubscribeWithRetryAsyncAction::AttemptSubscribe()
{
	if (bHasResolved || bCancellationRequested)
	{
		return;
	}

	++AttemptCounter;

	UMoqSubscriber* Subscriber = nullptr;
	if (ClientPtr.IsValid())
	{
		Subscriber = ClientPtr->Subscribe(NamespaceValue, TrackValue);
	}

	if (Subscriber)
	{
		FinishSuccess(Subscriber);
		return;
	}

	if (AttemptCounter >= MaxAttempts)
	{
		FinishFailure(DescribeLastError());
		return;
	}

	NextAttemptTime = FPlatformTime::Seconds() + RetryDelaySeconds;
	if (!RetryTickerHandle.IsValid())
	{
		RetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UMoqSubscribeWithRetryAsyncAction::HandleRetryTicker));
	}
}

bool UMoqSubscribeWithRetryAsyncAction::HandleRetryTicker(float)
{
	if (bHasResolved || bCancellationRequested)
	{
		return false;
	}

	if (FPlatformTime::Seconds() >= NextAttemptTime)
	{
		AttemptSubscribe();
	}

	return !bHasResolved;
}

void UMoqSubscribeWithRetryAsyncAction::FinishSuccess(UMoqSubscriber* Subscriber)
{
	if (bHasResolved)
	{
		return;
	}

	bHasResolved = true;
	OnSubscribed.Broadcast(Subscriber);
	Cleanup();
}

void UMoqSubscribeWithRetryAsyncAction::FinishFailure(const FString& ErrorMessage)
{
	if (bHasResolved)
	{
		return;
	}

	bHasResolved = true;
	OnFailed.Broadcast(ClientPtr.Get(), ErrorMessage);
	Cleanup();
}

FString UMoqSubscribeWithRetryAsyncAction::DescribeLastError() const
{
	const char* LastError = moq_last_error();
	const FString LastErrorMessage = LastError ? UTF8_TO_TCHAR(LastError) : TEXT("Unknown MoQ error");
	return FString::Printf(TEXT("Subscribe failed after %d attempt(s): %s"), AttemptCounter, *LastErrorMessage);
}

void UMoqSubscribeWithRetryAsyncAction::Cleanup()
{
	if (RetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RetryTickerHandle);
		RetryTickerHandle = FTSTicker::FDelegateHandle();
	}

	UnregisterWorldCleanupListener();
	bCancellationRequested = false;

	SetReadyToDestroy();
}

void UMoqSubscribeWithRetryAsyncAction::RegisterWorldCleanupListener()
{
	if (WorldCleanupHandle.IsValid())
	{
		return;
	}

	if (ResolveWorld())
	{
		WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UMoqSubscribeWithRetryAsyncAction::HandleWorldCleanup);
	}
}

void UMoqSubscribeWithRetryAsyncAction::UnregisterWorldCleanupListener()
{
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}
}

void UMoqSubscribeWithRetryAsyncAction::HandleWorldCleanup(UWorld* World, bool, bool)
{
	if (!CachedWorld.IsValid() || CachedWorld.Get() != World)
	{
		return;
	}

	if (!bHasResolved)
	{
		FinishFailure(TEXT("World cleaned up before subscription completed"));
	}
}

UWorld* UMoqSubscribeWithRetryAsyncAction::ResolveWorld() const
{
	if (CachedWorld.IsValid())
	{
		return CachedWorld.Get();
	}

	UObject* Context = WeakWorldContext.Get();
	if (!Context)
	{
		return nullptr;
	}

	UWorld* World = Context->GetWorld();
	if (!World && GEngine)
	{
		World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull);
	}

	if (World)
	{
		const_cast<UMoqSubscribeWithRetryAsyncAction*>(this)->CachedWorld = World;
	}

	return World;
}
