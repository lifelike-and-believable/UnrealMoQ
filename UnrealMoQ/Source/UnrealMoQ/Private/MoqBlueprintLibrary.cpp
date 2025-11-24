// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqBlueprintLibrary.h"
#include "MoqClient.h"
#include "UObject/Package.h"
#include "moq_ffi.h"

namespace
{
bool IsValidUtf8(const uint8* Data, int32 Length)
{
	if (Length <= 0 || Data == nullptr)
	{
		return true;
	}

	const uint8* Ptr = Data;
	const uint8* End = Data + Length;

	while (Ptr < End)
	{
		const uint8 Byte = *Ptr++;
		if ((Byte & 0x80) == 0)
		{
			continue; // ASCII
		}

		int32 ExpectedTrailing = 0;
		uint32 CodePoint = 0;

		if ((Byte & 0xE0) == 0xC0)
		{
			ExpectedTrailing = 1;
			CodePoint = Byte & 0x1F;
			if (CodePoint == 0)
			{
				return false; // Overlong encoding for ASCII
			}
		}
		else if ((Byte & 0xF0) == 0xE0)
		{
			ExpectedTrailing = 2;
			CodePoint = Byte & 0x0F;
		}
		else if ((Byte & 0xF8) == 0xF0)
		{
			ExpectedTrailing = 3;
			CodePoint = Byte & 0x07;
		}
		else
		{
			return false; // Invalid leading byte
		}

		if (Ptr + ExpectedTrailing > End)
		{
			return false; // Truncated sequence
		}

		for (int32 Index = 0; Index < ExpectedTrailing; ++Index)
		{
			const uint8 Trail = *Ptr++;
			if ((Trail & 0xC0) != 0x80)
			{
				return false; // Invalid continuation byte
			}
			CodePoint = (CodePoint << 6) | (Trail & 0x3F);
		}

		// Reject overlong encodings
		if ((ExpectedTrailing == 1 && CodePoint < 0x80) ||
			(ExpectedTrailing == 2 && CodePoint < 0x800) ||
			(ExpectedTrailing == 3 && CodePoint < 0x10000))
		{
			return false;
		}

		if (CodePoint > 0x10FFFF || (CodePoint >= 0xD800 && CodePoint <= 0xDFFF))
		{
			return false; // Outside Unicode range or surrogate pairs
		}
	}

	return true;
}
}

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

	if (!IsValidUtf8(Data.GetData(), Data.Num()))
	{
		UE_LOG(LogTemp, Warning, TEXT("BytesToString: Invalid UTF-8 sequences detected (pre-validation)"));
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

UMoqClient* UMoqBlueprintLibrary::CreateMoqClient(UObject* Outer)
{
	UObject* SafeOuter = Outer ? Outer : GetTransientPackage();

	if (!SafeOuter)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateMoqClient: Unable to determine a valid outer"));
		return nullptr;
	}

	UMoqClient* Client = NewObject<UMoqClient>(SafeOuter);

	if (!Client)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateMoqClient: Failed to instantiate UMoqClient"));
	}

	return Client;
}
