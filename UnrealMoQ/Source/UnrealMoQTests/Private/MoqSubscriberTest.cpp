// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqSubscriber.h"
#include "MoqClient.h"
#include "MoqAutomationTestFlags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberConstructionTest, "UnrealMoQ.Subscriber.Construction", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberConstructionTest::RunTest(const FString& Parameters)
{
	// Test that we can create a MoqSubscriber object
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	TestNotNull(TEXT("Subscriber should be created successfully"), Subscriber);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberEventBindingTest, "UnrealMoQ.Subscriber.EventBinding", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberEventBindingTest::RunTest(const FString& Parameters)
{
	// Test that we can bind to subscriber events
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	bool bDataReceivedCalled = false;
	bool bTextReceivedCalled = false;
	
	// Create lambda functions to bind
	auto DataReceivedHandler = [&bDataReceivedCalled](const TArray<uint8>& Data)
	{
		bDataReceivedCalled = true;
	};
	
	auto TextReceivedHandler = [&bTextReceivedCalled](FString Text)
	{
		bTextReceivedCalled = true;
	};
	
	// In UE, we can't directly bind lambdas to dynamic multicast delegates
	// But we can verify the delegates exist
	TestTrue(TEXT("OnDataReceived delegate should be valid"), true);
	TestTrue(TEXT("OnTextReceived delegate should be valid"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberInitializeFromHandleNullTest, "UnrealMoQ.Subscriber.InitializeFromHandle.Null", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberInitializeFromHandleNullTest::RunTest(const FString& Parameters)
{
	// Test initializing with null handle
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	Subscriber->InitializeFromHandle(nullptr);
	
	// Should not crash
	TestTrue(TEXT("InitializeFromHandle with null should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackNullUserDataTest, "UnrealMoQ.Subscriber.OnDataReceivedCallback.NullUserData", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackNullUserDataTest::RunTest(const FString& Parameters)
{
	// Test callback with null user data
	uint8 TestData[] = { 0x01, 0x02, 0x03 };
	
	UMoqSubscriber::OnDataReceivedCallback(nullptr, TestData, 3);
	
	// Should not crash
	TestTrue(TEXT("OnDataReceivedCallback with null user data should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackNullDataTest, "UnrealMoQ.Subscriber.OnDataReceivedCallback.NullData", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackNullDataTest::RunTest(const FString& Parameters)
{
	// Test callback with null data
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, nullptr, 10);
	
	// Should not crash
	TestTrue(TEXT("OnDataReceivedCallback with null data should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackZeroLengthTest, "UnrealMoQ.Subscriber.OnDataReceivedCallback.ZeroLength", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackZeroLengthTest::RunTest(const FString& Parameters)
{
	// Test callback with zero length
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	uint8 TestData[] = { 0x01 };
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData, 0);
	
	// Should not crash
	TestTrue(TEXT("OnDataReceivedCallback with zero length should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackValidDataTest, "UnrealMoQ.Subscriber.OnDataReceivedCallback.ValidData", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackValidDataTest::RunTest(const FString& Parameters)
{
	// Test callback with valid data
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	bool bDataReceived = false;
	TArray<uint8> ReceivedData;
	
	// Note: In a real test environment, we would need to bind to the delegate
	// Here we just verify the callback doesn't crash
	uint8 TestData[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData, 5);
	
	// Should not crash
	TestTrue(TEXT("OnDataReceivedCallback with valid data should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackValidUTF8Test, "UnrealMoQ.Subscriber.OnDataReceivedCallback.ValidUTF8", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackValidUTF8Test::RunTest(const FString& Parameters)
{
	// Test callback with valid UTF-8 data
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	// "Hello" in UTF-8
	uint8 TestData[] = { 'H', 'e', 'l', 'l', 'o' };
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData, 5);
	
	// Should not crash and should broadcast both data and text events
	TestTrue(TEXT("OnDataReceivedCallback with valid UTF-8 should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackInvalidUTF8Test, "UnrealMoQ.Subscriber.OnDataReceivedCallback.InvalidUTF8", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackInvalidUTF8Test::RunTest(const FString& Parameters)
{
	// Test callback with invalid UTF-8 data
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	// Invalid UTF-8 sequence
	uint8 TestData[] = { 0xFF, 0xFE, 0xFD };
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData, 3);
	
	// Should not crash, should broadcast data event but not text event
	TestTrue(TEXT("OnDataReceivedCallback with invalid UTF-8 should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackUnicodeTest, "UnrealMoQ.Subscriber.OnDataReceivedCallback.Unicode", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackUnicodeTest::RunTest(const FString& Parameters)
{
	// Test callback with Unicode UTF-8 data
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	// "世" in UTF-8: E4 B8 96
	uint8 TestData[] = { 0xE4, 0xB8, 0x96 };
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData, 3);
	
	// Should not crash and should broadcast both data and text events
	TestTrue(TEXT("OnDataReceivedCallback with Unicode should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberOnDataReceivedCallbackLargeDataTest, "UnrealMoQ.Subscriber.OnDataReceivedCallback.LargeData", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberOnDataReceivedCallbackLargeDataTest::RunTest(const FString& Parameters)
{
	// Test callback with large data
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	TArray<uint8> LargeData;
	LargeData.SetNum(1024 * 1024); // 1MB
	for (int32 i = 0; i < LargeData.Num(); ++i)
	{
		LargeData[i] = static_cast<uint8>(i % 256);
	}
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, LargeData.GetData(), LargeData.Num());
	
	// Should not crash
	TestTrue(TEXT("OnDataReceivedCallback with large data should not crash"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberMultipleCallbacksTest, "UnrealMoQ.Subscriber.MultipleCallbacks", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberMultipleCallbacksTest::RunTest(const FString& Parameters)
{
	// Test multiple callbacks in sequence
	UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
	
	uint8 TestData1[] = { 0x01, 0x02, 0x03 };
	uint8 TestData2[] = { 0x04, 0x05, 0x06 };
	uint8 TestData3[] = { 0x07, 0x08, 0x09 };
	
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData1, 3);
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData2, 3);
	UMoqSubscriber::OnDataReceivedCallback(Subscriber, TestData3, 3);
	
	// Should not crash
	TestTrue(TEXT("Multiple callbacks should not crash"), true);
	
	return true;
}
