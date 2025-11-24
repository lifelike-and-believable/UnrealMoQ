// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqBlueprintLibrary.h"
#include "moq_ffi.h"

FString UMoqBlueprintLibrary::GetMoqVersion()
{
	const char* Version = moq_version();
	if (Version)
	{
		return UTF8_TO_TCHAR(Version);
	}
	return TEXT("Unknown");
}

FString UMoqBlueprintLibrary::GetLastError()
{
	const char* Error = moq_last_error();
	if (Error)
	{
		return UTF8_TO_TCHAR(Error);
	}
	return FString();
}

FString UMoqBlueprintLibrary::BytesToString(const TArray<uint8>& Data)
{
	if (Data.Num() == 0)
	{
		return FString();
	}

	// Convert with UTF-8 validation
	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Data.GetData()), Data.Num());
	if (Converter.Length() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("BytesToString: Failed to convert UTF-8 data"));
		return FString();
	}

	FString Result(Converter.Length(), Converter.Get());
	
	// Check for replacement characters which indicate invalid UTF-8
	if (Result.Contains(TEXT("\uFFFD")))
	{
		UE_LOG(LogTemp, Warning, TEXT("BytesToString: Invalid UTF-8 sequences detected"));
		return FString();
	}

	return Result;
}

TArray<uint8> UMoqBlueprintLibrary::StringToBytes(const FString& Text)
{
	TArray<uint8> Result;

	if (Text.IsEmpty())
	{
		return Result;
	}

	FTCHARToUTF8 Converter(*Text);
	const uint8* Data = reinterpret_cast<const uint8*>(Converter.Get());
	Result.Append(Data, Converter.Length());

	return Result;
}
