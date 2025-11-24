// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqSubscriber.h"
#include "MoqClient.h"

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
	if (!UserData || !Data || DataLen == 0)
	{
		return;
	}
	
	UMoqSubscriber* Subscriber = static_cast<UMoqSubscriber*>(UserData);
	// Validate that the object is still valid
	if (!IsValid(Subscriber))
	{
		return;
	}

	// Copy data to TArray
	TArray<uint8> DataArray;
	DataArray.Append(Data, DataLen);

	// Try to decode as UTF-8 text with validation
	FString TextData;
	bool bIsValidText = false;

	// Validate UTF-8 by checking for valid conversion
	// FUTF8ToTCHAR performs validation during conversion
	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Data), DataLen);
	if (Converter.Length() > 0)
	{
		TextData = FString(Converter.Length(), Converter.Get());
		// Basic validation: check for replacement characters which indicate invalid UTF-8
		if (!TextData.Contains(TEXT("\uFFFD")))
		{
			bIsValidText = true;
		}
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
