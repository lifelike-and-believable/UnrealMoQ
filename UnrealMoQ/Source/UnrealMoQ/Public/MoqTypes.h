// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MoqTypes.generated.h"

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
struct UNREALMOQ_API FMoqResult
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
