// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqPublisher.h"
#include "MoqClient.h"

UMoqPublisher::UMoqPublisher()
	: PublisherHandle(nullptr)
{
}

UMoqPublisher::~UMoqPublisher()
{
	if (PublisherHandle)
	{
		moq_publisher_destroy(PublisherHandle);
		PublisherHandle = nullptr;
	}
}

void UMoqPublisher::BeginDestroy()
{
	if (PublisherHandle)
	{
		moq_publisher_destroy(PublisherHandle);
		PublisherHandle = nullptr;
	}

	Super::BeginDestroy();
}

void UMoqPublisher::InitializeFromHandle(MoqPublisher* Handle)
{
	PublisherHandle = Handle;
}

FMoqResult UMoqPublisher::PublishData(const TArray<uint8>& Data, EMoqDeliveryMode DeliveryMode)
{
	if (!PublisherHandle)
	{
		return FMoqResult(false, TEXT("Publisher not initialized"));
	}

	if (Data.Num() == 0)
	{
		return FMoqResult(false, TEXT("Cannot publish empty data"));
	}

	MoqDeliveryMode NativeDeliveryMode = (DeliveryMode == EMoqDeliveryMode::Datagram) ? MOQ_DELIVERY_DATAGRAM : MOQ_DELIVERY_STREAM;

	MoqResult Result = moq_publish_data(
		PublisherHandle,
		Data.GetData(),
		Data.Num(),
		NativeDeliveryMode
	);

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

FMoqResult UMoqPublisher::PublishText(const FString& Text, EMoqDeliveryMode DeliveryMode)
{
	if (!PublisherHandle)
	{
		return FMoqResult(false, TEXT("Publisher not initialized"));
	}

	if (Text.IsEmpty())
	{
		return FMoqResult(false, TEXT("Cannot publish empty text"));
	}

	// Convert to UTF-8
	FTCHARToUTF8 Converter(*Text);
	const uint8* Data = reinterpret_cast<const uint8*>(Converter.Get());
	size_t DataLen = Converter.Length();

	MoqDeliveryMode NativeDeliveryMode = (DeliveryMode == EMoqDeliveryMode::Datagram) ? MOQ_DELIVERY_DATAGRAM : MOQ_DELIVERY_STREAM;

	MoqResult Result = moq_publish_data(
		PublisherHandle,
		Data,
		DataLen,
		NativeDeliveryMode
	);

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
