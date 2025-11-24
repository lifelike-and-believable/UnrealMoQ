// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqClient.h"
#include "MoqPublisher.h"
#include "MoqSubscriber.h"

UMoqClient::UMoqClient()
	: ClientHandle(nullptr)
	, CurrentState(EMoqConnectionState::Disconnected)
{
}

UMoqClient::~UMoqClient()
{
	if (ClientHandle)
	{
		moq_client_destroy(ClientHandle);
		ClientHandle = nullptr;
	}
}

void UMoqClient::BeginDestroy()
{
	if (ClientHandle)
	{
		Disconnect();
		moq_client_destroy(ClientHandle);
		ClientHandle = nullptr;
	}

	Super::BeginDestroy();
}

FMoqResult UMoqClient::Connect(const FString& Url)
{
	// Create client if not already created
	if (!ClientHandle)
	{
		ClientHandle = moq_client_create();
		if (!ClientHandle)
		{
			return FMoqResult(false, TEXT("Failed to create MoQ client"));
		}
	}

	// Convert URL to ANSI
	FTCHARToUTF8 UrlConverter(*Url);
	const char* UrlCStr = UrlConverter.Get();

	// Connect to the relay
	MoqResult Result = moq_connect(ClientHandle, UrlCStr, &UMoqClient::OnConnectionStateChangedCallback, this);

	if (Result.code == MOQ_OK)
	{
		return FMoqResult(true);
	}
	else
	{
		FString ErrorMsg = UTF8_TO_TCHAR(Result.message);
		moq_free_str(Result.message);
		return FMoqResult(false, ErrorMsg);
	}
}

FMoqResult UMoqClient::Disconnect()
{
	if (!ClientHandle)
	{
		return FMoqResult(false, TEXT("Client not initialized"));
	}

	MoqResult Result = moq_disconnect(ClientHandle);

	if (Result.code == MOQ_OK)
	{
		CurrentState = EMoqConnectionState::Disconnected;
		OnConnectionStateChanged.Broadcast(CurrentState);
		return FMoqResult(true);
	}
	else
	{
		FString ErrorMsg = UTF8_TO_TCHAR(Result.message);
		moq_free_str(Result.message);
		return FMoqResult(false, ErrorMsg);
	}
}

bool UMoqClient::IsConnected() const
{
	if (!ClientHandle)
	{
		return false;
	}

	return moq_is_connected(ClientHandle);
}

FMoqResult UMoqClient::AnnounceNamespace(const FString& Namespace)
{
	if (!ClientHandle)
	{
		return FMoqResult(false, TEXT("Client not initialized"));
	}

	FTCHARToUTF8 NamespaceConverter(*Namespace);
	const char* NamespaceCStr = NamespaceConverter.Get();

	MoqResult Result = moq_announce_namespace(ClientHandle, NamespaceCStr);

	if (Result.code == MOQ_OK)
	{
		return FMoqResult(true);
	}
	else
	{
		FString ErrorMsg = UTF8_TO_TCHAR(Result.message);
		moq_free_str(Result.message);
		return FMoqResult(false, ErrorMsg);
	}
}

UMoqPublisher* UMoqClient::CreatePublisher(const FString& Namespace, const FString& TrackName, EMoqDeliveryMode DeliveryMode)
{
	if (!ClientHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot create publisher: Client not initialized"));
		return nullptr;
	}

	FTCHARToUTF8 NamespaceConverter(*Namespace);
	FTCHARToUTF8 TrackNameConverter(*TrackName);

	MoqDeliveryMode NativeDeliveryMode = (DeliveryMode == EMoqDeliveryMode::Datagram) ? MOQ_DELIVERY_DATAGRAM : MOQ_DELIVERY_STREAM;

	MoqPublisher* PublisherHandle = moq_create_publisher_ex(
		ClientHandle,
		NamespaceConverter.Get(),
		TrackNameConverter.Get(),
		NativeDeliveryMode
	);

	if (!PublisherHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create publisher for %s/%s"), *Namespace, *TrackName);
		return nullptr;
	}

	// Create UObject wrapper
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>(this);
	Publisher->InitializeFromHandle(PublisherHandle);

	return Publisher;
}

UMoqSubscriber* UMoqClient::Subscribe(const FString& Namespace, const FString& TrackName)
{
	if (!ClientHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot subscribe: Client not initialized"));
		return nullptr;
	}

	// Create UObject wrapper first so we can pass it as user data
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>(this);
	
	FTCHARToUTF8 NamespaceConverter(*Namespace);
	FTCHARToUTF8 TrackNameConverter(*TrackName);

	MoqSubscriber* SubscriberHandle = moq_subscribe(
		ClientHandle,
		NamespaceConverter.Get(),
		TrackNameConverter.Get(),
		&UMoqSubscriber::OnDataReceivedCallback,
		Subscriber
	);

	if (!SubscriberHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to subscribe to %s/%s"), *Namespace, *TrackName);
		return nullptr;
	}

	Subscriber->InitializeFromHandle(SubscriberHandle);

	return Subscriber;
}

void UMoqClient::OnConnectionStateChangedCallback(void* UserData, MoqConnectionState State)
{
	UMoqClient* Client = static_cast<UMoqClient*>(UserData);
	if (!Client)
	{
		return;
	}

	// Safely convert native state to Unreal enum
	EMoqConnectionState NewState;
	switch (State)
	{
	case MOQ_STATE_DISCONNECTED:
		NewState = EMoqConnectionState::Disconnected;
		break;
	case MOQ_STATE_CONNECTING:
		NewState = EMoqConnectionState::Connecting;
		break;
	case MOQ_STATE_CONNECTED:
		NewState = EMoqConnectionState::Connected;
		break;
	case MOQ_STATE_FAILED:
		NewState = EMoqConnectionState::Failed;
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("Unknown MoQ connection state: %d"), (int)State);
		return;
	}
	
	Client->CurrentState = NewState;

	// Broadcast on game thread
	AsyncTask(ENamedThreads::GameThread, [Client, NewState]()
	{
		if (IsValid(Client))
		{
			Client->OnConnectionStateChanged.Broadcast(NewState);
		}
	});
}

void UMoqClient::OnTrackAnnouncedCallback(void* UserData, const char* Namespace, const char* TrackName)
{
	UMoqClient* Client = static_cast<UMoqClient*>(UserData);
	if (!Client)
	{
		return;
	}

	FString NamespaceStr = UTF8_TO_TCHAR(Namespace);
	FString TrackNameStr = UTF8_TO_TCHAR(TrackName);

	// Broadcast on game thread
	AsyncTask(ENamedThreads::GameThread, [Client, NamespaceStr, TrackNameStr]()
	{
		if (IsValid(Client))
		{
			Client->OnTrackAnnounced.Broadcast(NamespaceStr, TrackNameStr);
		}
	});
}
