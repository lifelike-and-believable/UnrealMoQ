// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqBlueprintLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryGetMoqVersionTest, "UnrealMoQ.BlueprintLibrary.GetMoqVersion", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryGetMoqVersionTest::RunTest(const FString& Parameters)
{
	// Test that GetMoqVersion returns a non-empty string
	FString Version = UMoqBlueprintLibrary::GetMoqVersion();
	
	TestFalse(TEXT("GetMoqVersion should return a non-empty string"), Version.IsEmpty());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryGetLastErrorTest, "UnrealMoQ.BlueprintLibrary.GetLastError", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryGetLastErrorTest::RunTest(const FString& Parameters)
{
	// Test that GetLastError doesn't crash and returns a string (may be empty)
	FString Error = UMoqBlueprintLibrary::GetLastError();
	
	// We just verify it doesn't crash - error string may be empty or not depending on state
	TestTrue(TEXT("GetLastError should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryStringToBytesEmptyTest, "UnrealMoQ.BlueprintLibrary.StringToBytes.Empty", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryStringToBytesEmptyTest::RunTest(const FString& Parameters)
{
	// Test empty string conversion
	TArray<uint8> Result = UMoqBlueprintLibrary::StringToBytes(TEXT(""));
	
	TestEqual(TEXT("Empty string should result in empty array"), Result.Num(), 0);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryStringToBytesValidTest, "UnrealMoQ.BlueprintLibrary.StringToBytes.Valid", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryStringToBytesValidTest::RunTest(const FString& Parameters)
{
	// Test valid ASCII string conversion
	FString TestString = TEXT("Hello World");
	TArray<uint8> Result = UMoqBlueprintLibrary::StringToBytes(TestString);
	
	TestTrue(TEXT("Valid string should result in non-empty array"), Result.Num() > 0);
	TestEqual(TEXT("Result length should match expected UTF-8 length"), Result.Num(), 11);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryStringToBytesUnicodeTest, "UnrealMoQ.BlueprintLibrary.StringToBytes.Unicode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryStringToBytesUnicodeTest::RunTest(const FString& Parameters)
{
	// Test Unicode string conversion
	FString TestString = TEXT("Hello 世界");
	TArray<uint8> Result = UMoqBlueprintLibrary::StringToBytes(TestString);
	
	TestTrue(TEXT("Unicode string should result in non-empty array"), Result.Num() > 0);
	// "Hello " is 6 bytes, "世界" is 6 bytes in UTF-8 (3 bytes each)
	TestEqual(TEXT("Result length should match expected UTF-8 length for Unicode"), Result.Num(), 12);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryBytesToStringEmptyTest, "UnrealMoQ.BlueprintLibrary.BytesToString.Empty", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryBytesToStringEmptyTest::RunTest(const FString& Parameters)
{
	// Test empty array conversion
	TArray<uint8> EmptyArray;
	FString Result = UMoqBlueprintLibrary::BytesToString(EmptyArray);
	
	TestTrue(TEXT("Empty array should result in empty string"), Result.IsEmpty());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryBytesToStringValidTest, "UnrealMoQ.BlueprintLibrary.BytesToString.Valid", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryBytesToStringValidTest::RunTest(const FString& Parameters)
{
	// Test valid UTF-8 bytes conversion
	TArray<uint8> TestData = { 'H', 'e', 'l', 'l', 'o' };
	FString Result = UMoqBlueprintLibrary::BytesToString(TestData);
	
	TestEqual(TEXT("Valid UTF-8 bytes should convert correctly"), Result, TEXT("Hello"));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryBytesToStringUnicodeTest, "UnrealMoQ.BlueprintLibrary.BytesToString.Unicode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryBytesToStringUnicodeTest::RunTest(const FString& Parameters)
{
	// Test valid Unicode UTF-8 bytes conversion
	// "世" = E4 B8 96 in UTF-8
	TArray<uint8> TestData = { 0xE4, 0xB8, 0x96 };
	FString Result = UMoqBlueprintLibrary::BytesToString(TestData);
	
	TestEqual(TEXT("Valid Unicode UTF-8 bytes should convert correctly"), Result, TEXT("世"));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryBytesToStringInvalidUTF8Test, "UnrealMoQ.BlueprintLibrary.BytesToString.InvalidUTF8", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryBytesToStringInvalidUTF8Test::RunTest(const FString& Parameters)
{
	// Test invalid UTF-8 bytes (incomplete sequence)
	TArray<uint8> InvalidData = { 0xFF, 0xFE };
	FString Result = UMoqBlueprintLibrary::BytesToString(InvalidData);
	
	// Should return empty string for invalid UTF-8
	TestTrue(TEXT("Invalid UTF-8 bytes should result in empty string"), Result.IsEmpty());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryRoundTripTest, "UnrealMoQ.BlueprintLibrary.RoundTrip", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryRoundTripTest::RunTest(const FString& Parameters)
{
	// Test round-trip conversion: String -> Bytes -> String
	FString OriginalString = TEXT("Test String 123");
	TArray<uint8> Bytes = UMoqBlueprintLibrary::StringToBytes(OriginalString);
	FString ResultString = UMoqBlueprintLibrary::BytesToString(Bytes);
	
	TestEqual(TEXT("Round-trip conversion should preserve string"), ResultString, OriginalString);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryRoundTripUnicodeTest, "UnrealMoQ.BlueprintLibrary.RoundTripUnicode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqBlueprintLibraryRoundTripUnicodeTest::RunTest(const FString& Parameters)
{
	// Test round-trip conversion with Unicode: String -> Bytes -> String
	FString OriginalString = TEXT("Test 世界 123 🎮");
	TArray<uint8> Bytes = UMoqBlueprintLibrary::StringToBytes(OriginalString);
	FString ResultString = UMoqBlueprintLibrary::BytesToString(Bytes);
	
	TestEqual(TEXT("Round-trip conversion with Unicode should preserve string"), ResultString, OriginalString);
	
	return true;
}
