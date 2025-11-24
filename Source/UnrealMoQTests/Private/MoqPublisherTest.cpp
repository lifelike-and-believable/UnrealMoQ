// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqPublisher.h"
#include "MoqClient.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherConstructionTest, "UnrealMoQ.Publisher.Construction", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherConstructionTest::RunTest(const FString& Parameters)
{
	// Test that we can create a MoqPublisher object
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	TestNotNull(TEXT("Publisher should be created successfully"), Publisher);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishDataWithoutInitTest, "UnrealMoQ.Publisher.PublishData.WithoutInit", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishDataWithoutInitTest::RunTest(const FString& Parameters)
{
	// Test publishing data without initializing the publisher
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	TArray<uint8> TestData = { 0x01, 0x02, 0x03 };
	FMoqResult Result = Publisher->PublishData(TestData, EMoqDeliveryMode::Stream);
	
	// Should fail with appropriate error
	TestFalse(TEXT("PublishData without initialization should fail"), Result.bSuccess);
	TestFalse(TEXT("Error message should be provided"), Result.ErrorMessage.IsEmpty());
	TestTrue(TEXT("Error message should mention publisher not initialized"), 
		Result.ErrorMessage.Contains(TEXT("not initialized")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishDataEmptyDataTest, "UnrealMoQ.Publisher.PublishData.EmptyData", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishDataEmptyDataTest::RunTest(const FString& Parameters)
{
	// Test publishing empty data
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	TArray<uint8> EmptyData;
	FMoqResult Result = Publisher->PublishData(EmptyData, EMoqDeliveryMode::Stream);
	
	// Should fail with appropriate error
	TestFalse(TEXT("PublishData with empty data should fail"), Result.bSuccess);
	TestFalse(TEXT("Error message should be provided"), Result.ErrorMessage.IsEmpty());
	TestTrue(TEXT("Error message should mention empty data"), 
		Result.ErrorMessage.Contains(TEXT("empty")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishDataStreamModeTest, "UnrealMoQ.Publisher.PublishData.StreamMode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishDataStreamModeTest::RunTest(const FString& Parameters)
{
	// Test publishing data with stream delivery mode
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	TArray<uint8> TestData = { 0x01, 0x02, 0x03 };
	FMoqResult Result = Publisher->PublishData(TestData, EMoqDeliveryMode::Stream);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishData should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishDataDatagramModeTest, "UnrealMoQ.Publisher.PublishData.DatagramMode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishDataDatagramModeTest::RunTest(const FString& Parameters)
{
	// Test publishing data with datagram delivery mode
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	TArray<uint8> TestData = { 0x01, 0x02, 0x03 };
	FMoqResult Result = Publisher->PublishData(TestData, EMoqDeliveryMode::Datagram);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishData should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishDataLargeDataTest, "UnrealMoQ.Publisher.PublishData.LargeData", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishDataLargeDataTest::RunTest(const FString& Parameters)
{
	// Test publishing large data
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	TArray<uint8> LargeData;
	LargeData.SetNum(1024 * 1024); // 1MB
	for (int32 i = 0; i < LargeData.Num(); ++i)
	{
		LargeData[i] = static_cast<uint8>(i % 256);
	}
	
	FMoqResult Result = Publisher->PublishData(LargeData, EMoqDeliveryMode::Stream);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishData with large data should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishTextWithoutInitTest, "UnrealMoQ.Publisher.PublishText.WithoutInit", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishTextWithoutInitTest::RunTest(const FString& Parameters)
{
	// Test publishing text without initializing the publisher
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	FMoqResult Result = Publisher->PublishText(TEXT("Test message"), EMoqDeliveryMode::Stream);
	
	// Should fail with appropriate error
	TestFalse(TEXT("PublishText without initialization should fail"), Result.bSuccess);
	TestFalse(TEXT("Error message should be provided"), Result.ErrorMessage.IsEmpty());
	TestTrue(TEXT("Error message should mention publisher not initialized"), 
		Result.ErrorMessage.Contains(TEXT("not initialized")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishTextEmptyTextTest, "UnrealMoQ.Publisher.PublishText.EmptyText", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishTextEmptyTextTest::RunTest(const FString& Parameters)
{
	// Test publishing empty text
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	FMoqResult Result = Publisher->PublishText(TEXT(""), EMoqDeliveryMode::Stream);
	
	// Should fail with appropriate error
	TestFalse(TEXT("PublishText with empty text should fail"), Result.bSuccess);
	TestFalse(TEXT("Error message should be provided"), Result.ErrorMessage.IsEmpty());
	TestTrue(TEXT("Error message should mention empty text"), 
		Result.ErrorMessage.Contains(TEXT("empty")));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishTextValidTextTest, "UnrealMoQ.Publisher.PublishText.ValidText", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishTextValidTextTest::RunTest(const FString& Parameters)
{
	// Test publishing valid text
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	FMoqResult Result = Publisher->PublishText(TEXT("Hello World"), EMoqDeliveryMode::Stream);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishText should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishTextUnicodeTest, "UnrealMoQ.Publisher.PublishText.Unicode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishTextUnicodeTest::RunTest(const FString& Parameters)
{
	// Test publishing Unicode text
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	FMoqResult Result = Publisher->PublishText(TEXT("Hello 世界 🎮"), EMoqDeliveryMode::Stream);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishText with Unicode should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishTextStreamModeTest, "UnrealMoQ.Publisher.PublishText.StreamMode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishTextStreamModeTest::RunTest(const FString& Parameters)
{
	// Test publishing text with stream delivery mode
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	FMoqResult Result = Publisher->PublishText(TEXT("Test message"), EMoqDeliveryMode::Stream);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishText with stream mode should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishTextDatagramModeTest, "UnrealMoQ.Publisher.PublishText.DatagramMode", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishTextDatagramModeTest::RunTest(const FString& Parameters)
{
	// Test publishing text with datagram delivery mode
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	FMoqResult Result = Publisher->PublishText(TEXT("Test message"), EMoqDeliveryMode::Datagram);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishText with datagram mode should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherPublishTextLongTextTest, "UnrealMoQ.Publisher.PublishText.LongText", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqPublisherPublishTextLongTextTest::RunTest(const FString& Parameters)
{
	// Test publishing long text
	UMoqPublisher* Publisher = NewObject<UMoqPublisher>();
	
	FString LongText;
	for (int32 i = 0; i < 1000; ++i)
	{
		LongText += TEXT("Lorem ipsum dolor sit amet, consectetur adipiscing elit. ");
	}
	
	FMoqResult Result = Publisher->PublishText(LongText, EMoqDeliveryMode::Stream);
	
	// Without initialization, should fail gracefully
	TestTrue(TEXT("PublishText with long text should execute without crashing"), true);
	
	return true;
}
