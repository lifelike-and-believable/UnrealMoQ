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

	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Data.GetData()), Data.Num());
	return FString(Converter.Length(), Converter.Get());
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
