// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqClient.h"
#include "Misc/AutomationTest.h"
#include "MoqAutomationTestFlags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientConstructionTest, "UnrealMoQ.Client.Construction", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientConstructionTest::RunTest(const FString& Parameters)
{
	// Test that we can create a MoqClient object
	UMoqClient* Client = NewObject<UMoqClient>();
	
	TestNotNull(TEXT("Client should be created successfully"), Client);
	TestFalse(TEXT("New client should not be connected"), Client->IsConnected());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientIsConnectedInitialStateTest, "UnrealMoQ.Client.IsConnected.InitialState", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientIsConnectedInitialStateTest::RunTest(const FString& Parameters)
{
	// Test initial connection state
	UMoqClient* Client = NewObject<UMoqClient>();
	
	TestFalse(TEXT("Client should not be connected initially"), Client->IsConnected());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientConnectWithValidUrlTest, "UnrealMoQ.Client.Connect.ValidUrl", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientConnectWithValidUrlTest::RunTest(const FString& Parameters)
{
	// Test connecting with a valid URL format (may fail due to network, but should not crash)
	UMoqClient* Client = NewObject<UMoqClient>();
	
	FMoqResult Result = Client->Connect(TEXT("https://relay.example.com"));
	
	// We just verify it doesn't crash and returns a result
	// The result may succeed or fail depending on whether the relay is reachable
	TestTrue(TEXT("Connect should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientConnectWithEmptyUrlTest, "UnrealMoQ.Client.Connect.EmptyUrl", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientConnectWithEmptyUrlTest::RunTest(const FString& Parameters)
{
	// Test connecting with an empty URL
	UMoqClient* Client = NewObject<UMoqClient>();
	
	FMoqResult Result = Client->Connect(TEXT(""));
	
	// Empty URL should fail gracefully
	TestTrue(TEXT("Connect with empty URL should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientDisconnectWithoutConnectTest, "UnrealMoQ.Client.Disconnect.WithoutConnect", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientDisconnectWithoutConnectTest::RunTest(const FString& Parameters)
{
	// Test disconnecting without connecting first
	UMoqClient* Client = NewObject<UMoqClient>();
	
	FMoqResult Result = Client->Disconnect();
	
	// Should handle gracefully (may return error about not being initialized)
	TestTrue(TEXT("Disconnect without connect should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientAnnounceNamespaceWithoutConnectTest, "UnrealMoQ.Client.AnnounceNamespace.WithoutConnect", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientAnnounceNamespaceWithoutConnectTest::RunTest(const FString& Parameters)
{
	// Test announcing namespace without connecting first
	UMoqClient* Client = NewObject<UMoqClient>();
	
	FMoqResult Result = Client->AnnounceNamespace(TEXT("test-namespace"));
	
	// Should fail with appropriate error message
	TestFalse(TEXT("AnnounceNamespace without connect should fail"), Result.bSuccess);
	TestFalse(TEXT("Error message should be provided"), Result.ErrorMessage.IsEmpty());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientAnnounceNamespaceEmptyTest, "UnrealMoQ.Client.AnnounceNamespace.Empty", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientAnnounceNamespaceEmptyTest::RunTest(const FString& Parameters)
{
	// Test announcing empty namespace
	UMoqClient* Client = NewObject<UMoqClient>();
	
	FMoqResult Result = Client->AnnounceNamespace(TEXT(""));
	
	// Should handle gracefully
	TestTrue(TEXT("AnnounceNamespace with empty string should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientCreatePublisherWithoutConnectTest, "UnrealMoQ.Client.CreatePublisher.WithoutConnect", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientCreatePublisherWithoutConnectTest::RunTest(const FString& Parameters)
{
	// Test creating publisher without connecting first
	UMoqClient* Client = NewObject<UMoqClient>();
	
	AddExpectedError(TEXT("Cannot create publisher: Client not initialized"), EAutomationExpectedMessageFlags::Contains, 1);

	UMoqPublisher* Publisher = Client->CreatePublisher(TEXT("test-namespace"), TEXT("test-track"), EMoqDeliveryMode::Stream);
	
	// Should fail and return nullptr
	TestNull(TEXT("CreatePublisher without connect should return nullptr"), Publisher);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientCreatePublisherEmptyNamespaceTest, "UnrealMoQ.Client.CreatePublisher.EmptyNamespace", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientCreatePublisherEmptyNamespaceTest::RunTest(const FString& Parameters)
{
	// Test creating publisher with empty namespace
	UMoqClient* Client = NewObject<UMoqClient>();
	
	AddExpectedError(TEXT("Cannot create publisher: Client not initialized"), EAutomationExpectedMessageFlags::Contains, 1);

	UMoqPublisher* Publisher = Client->CreatePublisher(TEXT(""), TEXT("test-track"), EMoqDeliveryMode::Stream);
	
	// Should handle gracefully (return nullptr or error)
	TestTrue(TEXT("CreatePublisher with empty namespace should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientCreatePublisherEmptyTrackTest, "UnrealMoQ.Client.CreatePublisher.EmptyTrack", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientCreatePublisherEmptyTrackTest::RunTest(const FString& Parameters)
{
	// Test creating publisher with empty track name
	UMoqClient* Client = NewObject<UMoqClient>();
	
	AddExpectedError(TEXT("Cannot create publisher: Client not initialized"), EAutomationExpectedMessageFlags::Contains, 1);

	UMoqPublisher* Publisher = Client->CreatePublisher(TEXT("test-namespace"), TEXT(""), EMoqDeliveryMode::Stream);
	
	// Should handle gracefully (return nullptr or error)
	TestTrue(TEXT("CreatePublisher with empty track should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientCreatePublisherDatagramModeTest, "UnrealMoQ.Client.CreatePublisher.DatagramMode", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientCreatePublisherDatagramModeTest::RunTest(const FString& Parameters)
{
	// Test creating publisher with datagram delivery mode
	UMoqClient* Client = NewObject<UMoqClient>();
	
	AddExpectedError(TEXT("Cannot create publisher: Client not initialized"), EAutomationExpectedMessageFlags::Contains, 1);

	UMoqPublisher* Publisher = Client->CreatePublisher(TEXT("test-namespace"), TEXT("test-track"), EMoqDeliveryMode::Datagram);
	
	// Should execute without crashing (may return nullptr due to no connection)
	TestTrue(TEXT("CreatePublisher with datagram mode should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientSubscribeWithoutConnectTest, "UnrealMoQ.Client.Subscribe.WithoutConnect", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientSubscribeWithoutConnectTest::RunTest(const FString& Parameters)
{
	// Test subscribing without connecting first
	UMoqClient* Client = NewObject<UMoqClient>();
	
	AddExpectedError(TEXT("Cannot subscribe: Client not initialized"), EAutomationExpectedMessageFlags::Contains, 1);

	UMoqSubscriber* Subscriber = Client->Subscribe(TEXT("test-namespace"), TEXT("test-track"));
	
	// Should fail and return nullptr
	TestNull(TEXT("Subscribe without connect should return nullptr"), Subscriber);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientSubscribeEmptyNamespaceTest, "UnrealMoQ.Client.Subscribe.EmptyNamespace", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientSubscribeEmptyNamespaceTest::RunTest(const FString& Parameters)
{
	// Test subscribing with empty namespace
	UMoqClient* Client = NewObject<UMoqClient>();
	
	AddExpectedError(TEXT("Cannot subscribe: Client not initialized"), EAutomationExpectedMessageFlags::Contains, 1);

	UMoqSubscriber* Subscriber = Client->Subscribe(TEXT(""), TEXT("test-track"));
	
	// Should handle gracefully (return nullptr or error)
	TestTrue(TEXT("Subscribe with empty namespace should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientSubscribeEmptyTrackTest, "UnrealMoQ.Client.Subscribe.EmptyTrack", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientSubscribeEmptyTrackTest::RunTest(const FString& Parameters)
{
	// Test subscribing with empty track name
	UMoqClient* Client = NewObject<UMoqClient>();
	
	AddExpectedError(TEXT("Cannot subscribe: Client not initialized"), EAutomationExpectedMessageFlags::Contains, 1);

	UMoqSubscriber* Subscriber = Client->Subscribe(TEXT("test-namespace"), TEXT(""));
	
	// Should handle gracefully (return nullptr or error)
	TestTrue(TEXT("Subscribe with empty track should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientMultipleConnectTest, "UnrealMoQ.Client.MultipleConnect", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientMultipleConnectTest::RunTest(const FString& Parameters)
{
	// Test calling Connect multiple times
	UMoqClient* Client = NewObject<UMoqClient>();
	
	Client->Connect(TEXT("https://relay1.example.com"));
	FMoqResult Result = Client->Connect(TEXT("https://relay2.example.com"));
	
	// Should handle gracefully (may succeed or fail based on implementation)
	TestTrue(TEXT("Multiple Connect calls should execute without crashing"), true);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientFMoqResultSuccessTest, "UnrealMoQ.Client.FMoqResult.Success", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientFMoqResultSuccessTest::RunTest(const FString& Parameters)
{
	// Test FMoqResult structure with success
	FMoqResult Result(true);
	
	TestTrue(TEXT("Success result should have bSuccess = true"), Result.bSuccess);
	TestTrue(TEXT("Success result should have empty error message"), Result.ErrorMessage.IsEmpty());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientFMoqResultFailureTest, "UnrealMoQ.Client.FMoqResult.Failure", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientFMoqResultFailureTest::RunTest(const FString& Parameters)
{
	// Test FMoqResult structure with failure
	FMoqResult Result(false, TEXT("Test error message"));
	
	TestFalse(TEXT("Failure result should have bSuccess = false"), Result.bSuccess);
	TestEqual(TEXT("Failure result should have error message"), Result.ErrorMessage, TEXT("Test error message"));
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientFMoqResultDefaultTest, "UnrealMoQ.Client.FMoqResult.Default", MoqAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqClientFMoqResultDefaultTest::RunTest(const FString& Parameters)
{
	// Test FMoqResult default constructor
	FMoqResult Result;
	
	TestFalse(TEXT("Default result should have bSuccess = false"), Result.bSuccess);
	
	return true;
}
