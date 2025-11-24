// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "MoqBlueprintLibrary.h"
#include "MoqClient.h"
#include "MoqPublisher.h"
#include "MoqSubscriber.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "MoqAutomationTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqBlueprintLibraryStringConversionTest, "UnrealMoQ.BlueprintLibrary.StringConversions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientCreationAutomationTest, "UnrealMoQ.Client.Creation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FMoqCloudflarePublishSubscribeTest, FAutomationTestBase, "UnrealMoQ.Network.CloudflarePublishSubscribe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

struct FMoqRelayTestConfig
{
	FString RelayUrl = TEXT("https://relay.cloudflare.mediaoverquic.com");
	FString NamespacePrefix = TEXT("unrealmoq-e2e");
	FString TrackPrefix = TEXT("track");
	bool bIsEnabled = true;

	static FMoqRelayTestConfig Load()
	{
		FMoqRelayTestConfig Config;

		const FString UrlEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("MOQ_AUTOMATION_RELAY_URL"));
		if (!UrlEnv.IsEmpty())
		{
			Config.RelayUrl = UrlEnv;
		}

		const FString NamespaceEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("MOQ_AUTOMATION_NAMESPACE_PREFIX"));
		if (!NamespaceEnv.IsEmpty())
		{
			Config.NamespacePrefix = NamespaceEnv;
		}

		const FString TrackEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("MOQ_AUTOMATION_TRACK_PREFIX"));
		if (!TrackEnv.IsEmpty())
		{
			Config.TrackPrefix = TrackEnv;
		}

		const FString EnableEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("MOQ_AUTOMATION_ENABLE_NETWORK"));
		if (!EnableEnv.IsEmpty())
		{
			const bool bRequestedEnable = !EnableEnv.Equals(TEXT("0"), ESearchCase::IgnoreCase) && !EnableEnv.Equals(TEXT("false"), ESearchCase::IgnoreCase);
			Config.bIsEnabled = bRequestedEnable;
		}

		const FString SkipEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("MOQ_AUTOMATION_SKIP_NETWORK"));
		if (!SkipEnv.IsEmpty())
		{
			const bool bRequestedSkip = SkipEnv.Equals(TEXT("1"), ESearchCase::IgnoreCase) || SkipEnv.Equals(TEXT("true"), ESearchCase::IgnoreCase);
			if (bRequestedSkip)
			{
				Config.bIsEnabled = false;
			}
		}

		return Config;
	}
};

struct FMoqNetworkTestState : public TSharedFromThis<FMoqNetworkTestState>
{
	FString RelayUrl;
	FString Namespace;
	FString TrackName;
	FString ExpectedText;
	TArray<uint8> ExpectedBinary;

	TStrongObjectPtr<UMoqClient> PublisherClient;
	TStrongObjectPtr<UMoqClient> SubscriberClient;
	TStrongObjectPtr<UMoqPublisher> Publisher;
	TStrongObjectPtr<UMoqSubscriber> Subscriber;

	TStrongObjectPtr<UMoqAutomationEventSink> PublisherSink;
	TStrongObjectPtr<UMoqAutomationEventSink> SubscriberSink;

	bool bPublisherConnected = false;
	bool bSubscriberConnected = false;
	bool bTextReceived = false;
	bool bBinaryReceived = false;
	bool bEncounteredFailure = false;
	FString FailureReason;
};

void UMoqAutomationEventSink::Initialize(const TSharedPtr<FMoqNetworkTestState>& InState, bool bInIsPublisherHandler)
{
	WeakState = InState;
	bIsPublisherHandler = bInIsPublisherHandler;
}

void UMoqAutomationEventSink::HandleConnectionStateChanged(EMoqConnectionState NewState)
{
	if (const TSharedPtr<FMoqNetworkTestState> State = WeakState.Pin())
	{
		if (NewState == EMoqConnectionState::Failed)
		{
			State->bEncounteredFailure = true;
			State->FailureReason = bIsPublisherHandler ? TEXT("Publisher client reported failure state") : TEXT("Subscriber client reported failure state");
		}
		else if (NewState == EMoqConnectionState::Connected)
		{
			if (bIsPublisherHandler)
			{
				State->bPublisherConnected = true;
			}
			else
			{
				State->bSubscriberConnected = true;
			}
		}
	}
}

void UMoqAutomationEventSink::HandleSubscriberText(FString Text)
{
	if (const TSharedPtr<FMoqNetworkTestState> State = WeakState.Pin())
	{
		if (!State->ExpectedText.IsEmpty() && Text.Equals(State->ExpectedText, ESearchCase::CaseSensitive))
		{
			State->bTextReceived = true;
		}
	}
}

void UMoqAutomationEventSink::HandleSubscriberData(const TArray<uint8>& Data)
{
	if (const TSharedPtr<FMoqNetworkTestState> State = WeakState.Pin())
	{
		if (State->ExpectedBinary.Num() > 0 && State->ExpectedBinary == Data)
		{
			State->bBinaryReceived = true;
		}
	}
}

class FMoqLambdaLatentCommand : public IAutomationLatentCommand
{
public:
	explicit FMoqLambdaLatentCommand(TFunction<void()> InAction)
		: Action(MoveTemp(InAction))
		, bHasRun(false)
	{
	}

	virtual ~FMoqLambdaLatentCommand() override = default;

	virtual bool Update() override
	{
		if (!bHasRun)
		{
			Action();
			bHasRun = true;
		}
		return true;
	}

private:
	TFunction<void()> Action;
	bool bHasRun;
};

class FMoqWaitConditionLatentCommand : public IAutomationLatentCommand
{
public:
	FMoqWaitConditionLatentCommand(TFunction<bool()> InCondition,
		double InTimeoutSeconds,
		FAutomationTestBase* InTest,
		const TSharedPtr<FMoqNetworkTestState>& InState,
		FString InTimeoutError)
		: Condition(MoveTemp(InCondition))
		, TimeoutSeconds(InTimeoutSeconds)
		, StartTime(FPlatformTime::Seconds())
		, Test(InTest)
		, StateWeak(InState)
		, TimeoutError(MoveTemp(InTimeoutError))
	{
	}

	virtual ~FMoqWaitConditionLatentCommand() override = default;

	virtual bool Update() override
	{
		if (const TSharedPtr<FMoqNetworkTestState> State = StateWeak.Pin())
		{
			if (State->bEncounteredFailure)
			{
				Test->AddError(State->FailureReason);
				return true;
			}
		}

		if (Condition())
		{
			return true;
		}

		const double Now = FPlatformTime::Seconds();
		if ((Now - StartTime) >= TimeoutSeconds)
		{
			Test->AddError(TimeoutError);
			return true;
		}

		return false;
	}

private:
	TFunction<bool()> Condition;
	double TimeoutSeconds;
	double StartTime;
	FAutomationTestBase* Test;
	TWeakPtr<FMoqNetworkTestState> StateWeak;
	FString TimeoutError;
};

bool FMoqBlueprintLibraryStringConversionTest::RunTest(const FString& Parameters)
{
	const FString Original = TEXT("MoQ 🚀 こんにちは");
	const TArray<uint8> Bytes = UMoqBlueprintLibrary::StringToBytes(Original);

	if (!TestTrue(TEXT("StringToBytes should return data"), Bytes.Num() > 0))
	{
		return false;
	}

	const FString RoundTripped = UMoqBlueprintLibrary::BytesToString(Bytes);
	if (TestEqual(TEXT("BytesToString should decode UTF-8 payload"), RoundTripped, Original))
	{
		UE_LOG(LogTemp, Display, TEXT("BytesToString: Successfully round-tripped %d-byte UTF-8 payload"), Bytes.Num());
	}

	const FString SimpleAscii = TEXT("Media over QUIC");
	const TArray<uint8> SimpleBytes = UMoqBlueprintLibrary::StringToBytes(SimpleAscii);
	if (TestEqual(TEXT("BytesToString should decode ASCII payload"), UMoqBlueprintLibrary::BytesToString(SimpleBytes), SimpleAscii))
	{
		UE_LOG(LogTemp, Display, TEXT("BytesToString: ASCII sanity check succeeded"));
	}

	TArray<uint8> InvalidBytes = { 0xFF, 0xFE, 0xFD };
	const FString InvalidResult = UMoqBlueprintLibrary::BytesToString(InvalidBytes);
	if (TestTrue(TEXT("BytesToString should reject invalid UTF-8"), InvalidResult.IsEmpty()))
	{
		UE_LOG(LogTemp, Display, TEXT("BytesToString: Invalid UTF-8 payload correctly rejected"));
	}

	TArray<uint8> TruncatedBytes = { 0xE2, 0x82 };
	TestTrue(TEXT("BytesToString should reject truncated UTF-8"), UMoqBlueprintLibrary::BytesToString(TruncatedBytes).IsEmpty());

	TArray<uint8> EmptyBytes;
	const FString EmptyResult = UMoqBlueprintLibrary::BytesToString(EmptyBytes);
	TestTrue(TEXT("BytesToString should handle empty arrays"), EmptyResult.IsEmpty());

	const TArray<uint8> EmptyFromString = UMoqBlueprintLibrary::StringToBytes(TEXT(""));
	TestTrue(TEXT("StringToBytes should return empty data for empty string"), EmptyFromString.Num() == 0);

	return true;
}

bool FMoqClientCreationAutomationTest::RunTest(const FString& Parameters)
{
	UMoqClient* DefaultOuterClient = UMoqBlueprintLibrary::CreateMoqClient();
	if (!TestNotNull(TEXT("CreateMoqClient should return a valid client"), DefaultOuterClient))
	{
		return false;
	}

	TestEqual(TEXT("Client should default to transient outer"), DefaultOuterClient->GetOuter(), static_cast<UObject*>(GetTransientPackage()));
	TestFalse(TEXT("Client should not be connected immediately"), DefaultOuterClient->IsConnected());

	const FString UniquePackageName = FString::Printf(TEXT("/Temp/MoqAutomationOuter_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short));
	UPackage* CustomOuter = CreatePackage(*UniquePackageName);
	TestNotNull(TEXT("Custom outer package should be valid"), CustomOuter);
	CustomOuter->AddToRoot();

	UMoqClient* CustomOuterClient = UMoqBlueprintLibrary::CreateMoqClient(CustomOuter);
	TestNotNull(TEXT("CreateMoqClient should honor custom outer"), CustomOuterClient);
	TestEqual(TEXT("Client should use provided outer"), CustomOuterClient->GetOuter(), static_cast<UObject*>(CustomOuter));

	CustomOuter->RemoveFromRoot();
	CustomOuter->MarkAsGarbage();

	return true;
}

bool FMoqCloudflarePublishSubscribeTest::RunTest(const FString& Parameters)
{
	const FMoqRelayTestConfig Config = FMoqRelayTestConfig::Load();
	if (!Config.bIsEnabled)
	{
		AddWarning(TEXT("Skipping Cloudflare publish/subscribe test because MOQ_AUTOMATION_ENABLE_NETWORK=0 or MOQ_AUTOMATION_SKIP_NETWORK is set."));
		return true;
	}

	TSharedPtr<FMoqNetworkTestState> State = MakeShared<FMoqNetworkTestState>();
	State->RelayUrl = Config.RelayUrl;
	State->Namespace = FString::Printf(TEXT("%s-%s"), *Config.NamespacePrefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	State->TrackName = FString::Printf(TEXT("%s-%s"), *Config.TrackPrefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(12));
	State->ExpectedText = FString::Printf(TEXT("Automation-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	State->ExpectedBinary = { 0xDE, 0xAD, 0xBE, 0xEF, 0x42, 0x13 };

	UMoqClient* PublisherClientRaw = UMoqBlueprintLibrary::CreateMoqClient();
	if (!TestNotNull(TEXT("Publisher client created"), PublisherClientRaw))
	{
		return false;
	}
	State->PublisherClient.Reset(PublisherClientRaw);

	UMoqClient* SubscriberClientRaw = UMoqBlueprintLibrary::CreateMoqClient();
	if (!TestNotNull(TEXT("Subscriber client created"), SubscriberClientRaw))
	{
		return false;
	}
	State->SubscriberClient.Reset(SubscriberClientRaw);

	State->PublisherSink.Reset(NewObject<UMoqAutomationEventSink>());
	State->PublisherSink->Initialize(State, true);
	State->SubscriberSink.Reset(NewObject<UMoqAutomationEventSink>());
	State->SubscriberSink->Initialize(State, false);

	PublisherClientRaw->OnConnectionStateChanged.AddDynamic(State->PublisherSink.Get(), &UMoqAutomationEventSink::HandleConnectionStateChanged);
	SubscriberClientRaw->OnConnectionStateChanged.AddDynamic(State->SubscriberSink.Get(), &UMoqAutomationEventSink::HandleConnectionStateChanged);

	const FMoqResult PublisherConnectResult = PublisherClientRaw->Connect(State->RelayUrl);
	TestTrue(FString::Printf(TEXT("Connect publisher to %s"), *State->RelayUrl), PublisherConnectResult.bSuccess);

	const FMoqResult SubscriberConnectResult = SubscriberClientRaw->Connect(State->RelayUrl);
	TestTrue(FString::Printf(TEXT("Connect subscriber to %s"), *State->RelayUrl), SubscriberConnectResult.bSuccess);

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bPublisherConnected && State->bSubscriberConnected;
		},
		30.0,
		this,
		State,
		TEXT("Timed out waiting for both clients to connect to Cloudflare relay")));

	AddCommand(new FMoqLambdaLatentCommand([this, State]()
	{
		UMoqClient* PublisherClient = State->PublisherClient.Get();
		UMoqClient* SubscriberClient = State->SubscriberClient.Get();
		if (!PublisherClient || !SubscriberClient)
		{
			AddError(TEXT("Clients became invalid before namespace setup"));
			return;
		}

		const FMoqResult AnnounceResult = PublisherClient->AnnounceNamespace(State->Namespace);
		if (!AnnounceResult.bSuccess)
		{
			AddError(FString::Printf(TEXT("AnnounceNamespace failed: %s"), *AnnounceResult.ErrorMessage));
		}

		UMoqPublisher* Publisher = PublisherClient->CreatePublisher(State->Namespace, State->TrackName, EMoqDeliveryMode::Stream);
		if (!TestNotNull(TEXT("Publisher created"), Publisher))
		{
			return;
		}
		State->Publisher.Reset(Publisher);

		UMoqSubscriber* Subscriber = SubscriberClient->Subscribe(State->Namespace, State->TrackName);
		if (!TestNotNull(TEXT("Subscriber created"), Subscriber))
		{
			return;
		}
		State->Subscriber.Reset(Subscriber);

		Subscriber->OnTextReceived.AddDynamic(State->SubscriberSink.Get(), &UMoqAutomationEventSink::HandleSubscriberText);
		Subscriber->OnDataReceived.AddDynamic(State->SubscriberSink.Get(), &UMoqAutomationEventSink::HandleSubscriberData);
	}));

	AddCommand(new FMoqLambdaLatentCommand([this, State]()
	{
		UMoqPublisher* Publisher = State->Publisher.Get();
		if (!Publisher)
		{
			AddError(TEXT("Publisher is null before publishing payloads"));
			return;
		}

		const FMoqResult PublishTextResult = Publisher->PublishText(State->ExpectedText, EMoqDeliveryMode::Stream);
		if (!PublishTextResult.bSuccess)
		{
			AddError(FString::Printf(TEXT("PublishText failed: %s"), *PublishTextResult.ErrorMessage));
		}

		const FMoqResult PublishBinaryResult = Publisher->PublishData(State->ExpectedBinary, EMoqDeliveryMode::Stream);
		if (!PublishBinaryResult.bSuccess)
		{
			AddError(FString::Printf(TEXT("PublishData failed: %s"), *PublishBinaryResult.ErrorMessage));
		}
	}));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bTextReceived;
		},
		45.0,
		this,
		State,
		TEXT("Timed out waiting for subscriber to receive text payload")));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bBinaryReceived;
		},
		45.0,
		this,
		State,
		TEXT("Timed out waiting for subscriber to receive binary payload")));

	AddCommand(new FMoqLambdaLatentCommand([State]()
	{
		if (UMoqClient* PublisherClient = State->PublisherClient.Get())
		{
			PublisherClient->Disconnect();
		}
		if (UMoqClient* SubscriberClient = State->SubscriberClient.Get())
		{
			SubscriberClient->Disconnect();
		}
	}));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
