// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqSubscriber.h"

UMoqSubscriber::UMoqSubscriber()
	: SubscriberHandle(nullptr)
{
}

UMoqSubscriber::~UMoqSubscriber()
{
	if (SubscriberHandle)
	{
		moq_subscriber_destroy(SubscriberHandle);
		SubscriberHandle = nullptr;
	}
}

void UMoqSubscriber::BeginDestroy()
{
	if (SubscriberHandle)
	{
		moq_subscriber_destroy(SubscriberHandle);
		SubscriberHandle = nullptr;
	}

	Super::BeginDestroy();
}

void UMoqSubscriber::InitializeFromHandle(MoqSubscriber* Handle)
{
	SubscriberHandle = Handle;
}

void UMoqSubscriber::OnDataReceivedCallback(void* UserData, const uint8_t* Data, size_t DataLen)
{
	UMoqSubscriber* Subscriber = static_cast<UMoqSubscriber*>(UserData);
	if (!Subscriber || !Data || DataLen == 0)
	{
		return;
	}

	// Copy data to TArray
	TArray<uint8> DataArray;
	DataArray.Append(Data, DataLen);

	// Try to decode as UTF-8 text
	FString TextData;
	bool bIsValidText = false;

	// Create a UTF-8 string and try to convert
	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Data), DataLen);
	if (Converter.Length() > 0)
	{
		TextData = FString(Converter.Length(), Converter.Get());
		bIsValidText = true;
	}

	// Broadcast on game thread
	AsyncTask(ENamedThreads::GameThread, [Subscriber, DataArray, TextData, bIsValidText]()
	{
		if (IsValid(Subscriber))
		{
			// Always broadcast binary data
			Subscriber->OnDataReceived.Broadcast(DataArray);

			// Broadcast text if it was valid UTF-8
			if (bIsValidText)
			{
				Subscriber->OnTextReceived.Broadcast(TextData);
			}
		}
	});
}
