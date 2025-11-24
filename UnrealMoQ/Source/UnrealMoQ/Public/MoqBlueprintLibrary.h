// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MoqBlueprintLibrary.generated.h"

/**
 * Blueprint function library for MoQ utilities
 */
UCLASS()
class UNREALMOQ_API UMoqBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Get the MoQ FFI library version
	 * @return Version string
	 */
	UFUNCTION(BlueprintPure, Category = "MoQ|Utilities")
	static FString GetMoqVersion();

	/**
	 * Get the last error message from the MoQ library
	 * @return Last error message or empty string if no error
	 */
	UFUNCTION(BlueprintPure, Category = "MoQ|Utilities")
	static FString GetLastError();

	/**
	 * Convert byte array to string (UTF-8 decode)
	 * @param Data Byte array to convert
	 * @return Decoded string
	 */
	UFUNCTION(BlueprintPure, Category = "MoQ|Utilities")
	static FString BytesToString(const TArray<uint8>& Data);

	/**
	 * Convert string to byte array (UTF-8 encode)
	 * @param Text String to convert
	 * @return Encoded byte array
	 */
	UFUNCTION(BlueprintPure, Category = "MoQ|Utilities")
	static TArray<uint8> StringToBytes(const FString& Text);
};
