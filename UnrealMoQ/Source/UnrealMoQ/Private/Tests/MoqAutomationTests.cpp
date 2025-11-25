// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "MoqAutomationBlueprintLibrary.h"
#include "MoqBlueprintAsyncActions.h"
#include "MoqBlueprintLibrary.h"
#include "MoqClient.h"
#include "MoqPublisher.h"
#include "MoqSubscriber.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "MoqAutomationTestHelpers.h"
#include "Misc/CommandLine.h"
#include "moq_ffi.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqStringUtf8RoundTripTest, "UnrealMoQ.BlueprintLibrary.StringConversions.RoundTripUnicode", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqStringAsciiRoundTripTest, "UnrealMoQ.BlueprintLibrary.StringConversions.RoundTripASCII", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqStringRejectInvalidUtf8Test, "UnrealMoQ.BlueprintLibrary.StringConversions.RejectInvalidUtf8", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqStringRejectTruncatedUtf8Test, "UnrealMoQ.BlueprintLibrary.StringConversions.RejectTruncatedUtf8", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqStringEmptyInputTest, "UnrealMoQ.BlueprintLibrary.StringConversions.EmptyInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqClientCreationAutomationTest, "UnrealMoQ.Client.Creation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FMoqCloudflarePublishSubscribeTest, FAutomationTestBase, "UnrealMoQ.Network.CloudflarePublishSubscribe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FMoqCloudflareBlueprintPublishSubscribeTest, FAutomationTestBase, "UnrealMoQ.Network.CloudflareBlueprintPublishSubscribe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

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
			if (EnableEnv.Equals(TEXT("0"), ESearchCase::IgnoreCase) || EnableEnv.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				Config.bIsEnabled = false;
			}
			else if (EnableEnv.Equals(TEXT("1"), ESearchCase::IgnoreCase) || EnableEnv.Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				Config.bIsEnabled = true;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("MoqEnableNetworkAutomation")))
		{
			Config.bIsEnabled = true;
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

		if (FParse::Param(FCommandLine::Get(), TEXT("MoqSkipNetworkAutomation")) || FParse::Param(FCommandLine::Get(), TEXT("MoqDisableNetworkAutomation")))
		{
			Config.bIsEnabled = false;
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
	TStrongObjectPtr<UMoqConnectClientAsyncAction> PublisherConnectAction;
	TStrongObjectPtr<UMoqConnectClientAsyncAction> SubscriberConnectAction;
	TStrongObjectPtr<UMoqSubscribeWithRetryAsyncAction> SubscribeAsyncAction;

	TStrongObjectPtr<UMoqAutomationEventSink> PublisherSink;
	TStrongObjectPtr<UMoqAutomationEventSink> SubscriberSink;

	bool bPublisherConnected = false;
	bool bSubscriberConnected = false;
	bool bTextReceived = false;
	bool bBinaryReceived = false;
	bool bTrackPrimed = false;
	bool bEncounteredFailure = false;
	bool bPublisherConnectActionCompleted = false;
	bool bSubscriberConnectActionCompleted = false;
	bool bBlueprintSubscriberReady = false;
	int32 SubscribeAttempts = 0;
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

		if (!State->ExpectedText.IsEmpty())
		{
			const FString CandidateText = UMoqBlueprintLibrary::BytesToString(Data);
			if (CandidateText.Equals(State->ExpectedText, ESearchCase::CaseSensitive))
			{
				State->bTextReceived = true;
			}
		}
	}
}

void UMoqAutomationEventSink::HandleConnectAsyncSuccess(UMoqClient*)
{
	if (const TSharedPtr<FMoqNetworkTestState> State = WeakState.Pin())
	{
		if (bIsPublisherHandler)
		{
			State->bPublisherConnectActionCompleted = true;
		}
		else
		{
			State->bSubscriberConnectActionCompleted = true;
		}
	}
}

void UMoqAutomationEventSink::HandleConnectAsyncFailure(UMoqClient*, const FString& ErrorMessage)
{
	if (const TSharedPtr<FMoqNetworkTestState> State = WeakState.Pin())
	{
		State->bEncounteredFailure = true;
		State->FailureReason = ErrorMessage;
	}
}

void UMoqAutomationEventSink::HandleSubscribeAsyncSuccess(UMoqSubscriber* Subscriber)
{
	if (bIsPublisherHandler)
	{
		return;
	}

	if (const TSharedPtr<FMoqNetworkTestState> State = WeakState.Pin())
	{
		State->Subscriber.Reset(Subscriber);
		State->bBlueprintSubscriberReady = true;

		Subscriber->OnTextReceived.AddDynamic(this, &UMoqAutomationEventSink::HandleSubscriberText);
		Subscriber->OnDataReceived.AddDynamic(this, &UMoqAutomationEventSink::HandleSubscriberData);
	}
}

void UMoqAutomationEventSink::HandleSubscribeAsyncFailure(UMoqClient*, const FString& ErrorMessage)
{
	if (const TSharedPtr<FMoqNetworkTestState> State = WeakState.Pin())
	{
		State->bEncounteredFailure = true;
		State->FailureReason = ErrorMessage;
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

class FMoqDelayLatentCommand : public IAutomationLatentCommand
{
public:
	explicit FMoqDelayLatentCommand(double InSeconds)
		: DurationSeconds(InSeconds)
		, StartTime(FPlatformTime::Seconds())
	{
	}

	virtual bool Update() override
	{
		return (FPlatformTime::Seconds() - StartTime) >= DurationSeconds;
	}

private:
	double DurationSeconds = 0.0;
	double StartTime = 0.0;
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
		UMoqAutomationBlueprintLibrary::PumpMoqEventLoop();

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

class FMoqSubscribeRetryLatentCommand : public IAutomationLatentCommand
{
public:
	FMoqSubscribeRetryLatentCommand(
		const TSharedPtr<FMoqNetworkTestState>& InState,
		TFunction<void(const FString&)> InFailureCallback,
		FAutomationTestBase* InTest,
		int32 InMaxAttempts,
		double InRetryDelaySeconds)
		: StateWeak(InState)
		, FailureCallback(MoveTemp(InFailureCallback))
		, Test(InTest)
		, MaxAttempts(InMaxAttempts)
		, RetryDelaySeconds(InRetryDelaySeconds)
	{
	}

	virtual bool Update() override
	{
		const TSharedPtr<FMoqNetworkTestState> State = StateWeak.Pin();
		if (!State.IsValid())
		{
			return true;
		}

		if (State->bEncounteredFailure)
		{
			return true;
		}

		if (State->Subscriber.IsValid())
		{
			return true;
		}

		const double Now = FPlatformTime::Seconds();
		if (bWaitingForRetry && Now < NextAttemptTime)
		{
			return false;
		}

		bWaitingForRetry = false;
		++State->SubscribeAttempts;

		UMoqClient* SubscriberClient = State->SubscriberClient.Get();
		if (!SubscriberClient)
		{
			ReportFailure(TEXT("Subscriber client invalid before subscribe attempt"));
			return true;
		}

		UMoqSubscriber* Subscriber = SubscriberClient->Subscribe(State->Namespace, State->TrackName);
		if (Subscriber)
		{
			State->Subscriber.Reset(Subscriber);
			if (UMoqAutomationEventSink* Sink = State->SubscriberSink.Get())
			{
				Subscriber->OnTextReceived.AddDynamic(Sink, &UMoqAutomationEventSink::HandleSubscriberText);
				Subscriber->OnDataReceived.AddDynamic(Sink, &UMoqAutomationEventSink::HandleSubscriberData);
			}
			if (Test)
			{
				Test->AddInfo(FString::Printf(TEXT("Subscriber created after %d attempt(s)"), State->SubscribeAttempts));
			}
			return true;
		}

		if (State->SubscribeAttempts >= MaxAttempts)
		{
			ReportFailure(DescribeLastSubscribeError(State->Namespace, State->TrackName));
			return true;
		}

		if (Test)
		{
			Test->AddWarning(FString::Printf(TEXT("Subscribe attempt %d/%d failed for %s/%s. Retrying in %.2fs (%s)"),
				State->SubscribeAttempts,
				MaxAttempts,
				*State->Namespace,
				*State->TrackName,
				RetryDelaySeconds,
				*DescribeLastSubscribeError(State->Namespace, State->TrackName)));
		}

		bWaitingForRetry = true;
		NextAttemptTime = Now + RetryDelaySeconds;
		return false;
	}

private:
	void ReportFailure(const FString& Message)
	{
		if (Test)
		{
			Test->AddError(Message);
		}
		if (FailureCallback)
		{
			FailureCallback(Message);
		}
	}

	static FString DescribeLastSubscribeError(const FString& Namespace, const FString& TrackName)
	{
		const char* LastError = moq_last_error();
		const FString LastErrorMessage = LastError ? UTF8_TO_TCHAR(LastError) : TEXT("Unknown error");
		return FString::Printf(TEXT("Failed to subscribe to %s/%s: %s"), *Namespace, *TrackName, *LastErrorMessage);
	}

	TWeakPtr<FMoqNetworkTestState> StateWeak;
	TFunction<void(const FString&)> FailureCallback;
	FAutomationTestBase* Test = nullptr;
	int32 MaxAttempts = 1;
	double RetryDelaySeconds = 0.0;
	bool bWaitingForRetry = false;
	double NextAttemptTime = 0.0;
};

bool FMoqStringUtf8RoundTripTest::RunTest(const FString& Parameters)
{
	const FString Input = TEXT("MoQ 🚀 こんにちは");
	const TArray<uint8> Bytes = UMoqBlueprintLibrary::StringToBytes(Input);

	bool bSuccess = true;
	bSuccess &= TestTrue(TEXT("StringToBytes should produce data for Unicode input"), Bytes.Num() > 0);

	const FString RoundTripped = UMoqBlueprintLibrary::BytesToString(Bytes);
	bSuccess &= TestEqual(TEXT("BytesToString should reproduce the original Unicode string"), RoundTripped, Input);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("BytesToString: Successfully round-tripped %d-byte UTF-8 payload"), Bytes.Num());
	}

	return bSuccess;
}

bool FMoqStringAsciiRoundTripTest::RunTest(const FString& Parameters)
{
	const FString Input = TEXT("Media over QUIC");
	const TArray<uint8> Bytes = UMoqBlueprintLibrary::StringToBytes(Input);
	const FString RoundTripped = UMoqBlueprintLibrary::BytesToString(Bytes);

	const bool bSuccess = TestEqual(TEXT("ASCII payloads should round-trip losslessly"), RoundTripped, Input);
	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("BytesToString: ASCII sanity check succeeded"));
	}

	return bSuccess;
}

bool FMoqStringRejectInvalidUtf8Test::RunTest(const FString& Parameters)
{
	const TArray<uint8> InvalidBytes = { 0xFF, 0xFE, 0xFD };
	const FString Result = UMoqBlueprintLibrary::BytesToString(InvalidBytes);
	const bool bRejected = TestTrue(TEXT("BytesToString should reject invalid UTF-8 sequences"), Result.IsEmpty());

	if (bRejected)
	{
		UE_LOG(LogTemp, Display, TEXT("BytesToString: Invalid UTF-8 payload correctly rejected"));
	}

	return bRejected;
}

bool FMoqStringRejectTruncatedUtf8Test::RunTest(const FString& Parameters)
{
	const TArray<uint8> TruncatedBytes = { 0xE2, 0x82 };
	return TestTrue(TEXT("BytesToString should reject truncated UTF-8 sequences"), UMoqBlueprintLibrary::BytesToString(TruncatedBytes).IsEmpty());
}

bool FMoqStringEmptyInputTest::RunTest(const FString& Parameters)
{
	TArray<uint8> EmptyBytes;
	bool bSuccess = true;
	bSuccess &= TestTrue(TEXT("BytesToString should return an empty string for empty byte arrays"), UMoqBlueprintLibrary::BytesToString(EmptyBytes).IsEmpty());

	const TArray<uint8> EmptyFromString = UMoqBlueprintLibrary::StringToBytes(TEXT(""));
	bSuccess &= TestEqual(TEXT("StringToBytes should return zero bytes for empty strings"), 0, EmptyFromString.Num());

	return bSuccess;
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
		AddWarning(TEXT("Skipping Cloudflare publish/subscribe test. Set MOQ_AUTOMATION_ENABLE_NETWORK=1 or pass -MoqEnableNetworkAutomation to opt in."));
		return true;
	}

	const double ConnectTimeoutSeconds = 20.0;
	const double PayloadTimeoutSeconds = 30.0;
	const double TrackPrimeDelaySeconds = 0.5;
	const double PostSubscribeDelaySeconds = 0.25;
	const int32 MaxSubscribeAttempts = 5;
	const double SubscribeRetryDelaySeconds = 0.75;
	const TArray<uint8> TrackPrimePayload = []()
	{
		TArray<uint8> Data;
		Data.Add(0xAA);
		Data.Add(0x55);
		Data.Add(0xFF);
		return Data;
	}();

	TSharedPtr<FMoqNetworkTestState> State = MakeShared<FMoqNetworkTestState>();
	State->RelayUrl = Config.RelayUrl;
	State->Namespace = FString::Printf(TEXT("%s-%s"), *Config.NamespacePrefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	State->TrackName = FString::Printf(TEXT("%s-%s"), *Config.TrackPrefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(12));
	State->ExpectedText = FString::Printf(TEXT("Automation-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	State->ExpectedBinary = { 0xDE, 0xAD, 0xBE, 0xEF, 0x42, 0x13 };

	const auto MarkFailure = [State](const FString& Reason)
	{
		State->bEncounteredFailure = true;
		State->FailureReason = Reason;
	};

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
	if (!PublisherConnectResult.bSuccess)
	{
		const FString Error = FString::Printf(TEXT("Publisher failed to connect to %s: %s"), *State->RelayUrl, *PublisherConnectResult.ErrorMessage);
		AddError(Error);
		MarkFailure(Error);
		return false;
	}

	const FMoqResult SubscriberConnectResult = SubscriberClientRaw->Connect(State->RelayUrl);
	if (!SubscriberConnectResult.bSuccess)
	{
		const FString Error = FString::Printf(TEXT("Subscriber failed to connect to %s: %s"), *State->RelayUrl, *SubscriberConnectResult.ErrorMessage);
		AddError(Error);
		MarkFailure(Error);
		return false;
	}

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bPublisherConnected && State->bSubscriberConnected;
		},
		ConnectTimeoutSeconds,
		this,
		State,
		TEXT("Timed out waiting for both clients to connect to Cloudflare relay")));

	AddCommand(new FMoqLambdaLatentCommand([this, State, MarkFailure]()
	{
		UMoqClient* PublisherClient = State->PublisherClient.Get();
		if (!PublisherClient)
		{
			const FString Error = TEXT("Publisher client became invalid before namespace setup");
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		const FMoqResult AnnounceResult = PublisherClient->AnnounceNamespace(State->Namespace);
		if (!AnnounceResult.bSuccess)
		{
			const FString Error = FString::Printf(TEXT("AnnounceNamespace failed: %s"), *AnnounceResult.ErrorMessage);
			AddError(Error);
			MarkFailure(Error);
		}

		UMoqPublisher* Publisher = PublisherClient->CreatePublisher(State->Namespace, State->TrackName, EMoqDeliveryMode::Stream);
		if (!TestNotNull(TEXT("Publisher created"), Publisher))
		{
			MarkFailure(TEXT("Publisher creation failed"));
			return;
		}
		State->Publisher.Reset(Publisher);
	}));

	AddCommand(new FMoqLambdaLatentCommand([this, State, MarkFailure, TrackPrimePayload]()
	{
		UMoqPublisher* Publisher = State->Publisher.Get();
		if (!Publisher)
		{
			const FString Error = TEXT("Publisher is null before publishing payloads");
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		const FMoqResult PrimeResult = Publisher->PublishData(TrackPrimePayload, EMoqDeliveryMode::Stream);
		State->bTrackPrimed = PrimeResult.bSuccess;
		TestTrue(TEXT("Track priming publish succeeds"), PrimeResult.bSuccess);
	}));

	AddCommand(new FMoqDelayLatentCommand(TrackPrimeDelaySeconds));

	AddCommand(new FMoqLambdaLatentCommand([this, State, MarkFailure]()
	{
		if (!State->bTrackPrimed)
		{
			const FString Error = TEXT("Track priming failed before subscription attempts");
			AddError(Error);
			MarkFailure(Error);
		}
	}));

	AddCommand(new FMoqSubscribeRetryLatentCommand(State, MarkFailure, this, MaxSubscribeAttempts, SubscribeRetryDelaySeconds));
	AddCommand(new FMoqDelayLatentCommand(PostSubscribeDelaySeconds));

	AddCommand(new FMoqLambdaLatentCommand([this, State, MarkFailure]()
	{
		UMoqPublisher* Publisher = State->Publisher.Get();
		UMoqSubscriber* Subscriber = State->Subscriber.Get();
		if (!Publisher || !Subscriber)
		{
			const FString Error = TEXT("Publisher or subscriber invalid before verification publishes");
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		const FMoqResult PublishTextResult = Publisher->PublishText(State->ExpectedText, EMoqDeliveryMode::Stream);
		if (!PublishTextResult.bSuccess)
		{
			const FString Error = FString::Printf(TEXT("PublishText failed: %s"), *PublishTextResult.ErrorMessage);
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		const FMoqResult PublishBinaryResult = Publisher->PublishData(State->ExpectedBinary, EMoqDeliveryMode::Stream);
		if (!PublishBinaryResult.bSuccess)
		{
			const FString Error = FString::Printf(TEXT("PublishData failed: %s"), *PublishBinaryResult.ErrorMessage);
			AddError(Error);
			MarkFailure(Error);
			return;
		}
	}));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bTextReceived;
		},
		PayloadTimeoutSeconds,
		this,
		State,
		TEXT("Timed out waiting for subscriber to receive text payload")));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bBinaryReceived;
		},
		PayloadTimeoutSeconds,
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

		State->Publisher.Reset();
		State->Subscriber.Reset();
		State->PublisherClient.Reset();
		State->SubscriberClient.Reset();
	}));

	return true;
}

bool FMoqCloudflareBlueprintPublishSubscribeTest::RunTest(const FString& Parameters)
{
	const FMoqRelayTestConfig Config = FMoqRelayTestConfig::Load();
	if (!Config.bIsEnabled)
	{
		AddWarning(TEXT("Skipping Cloudflare Blueprint publish/subscribe test. Set MOQ_AUTOMATION_ENABLE_NETWORK=1 or pass -MoqEnableNetworkAutomation to opt in."));
		return true;
	}

	const double ConnectTimeoutSeconds = 20.0;
	const double PayloadTimeoutSeconds = 30.0;
	const double TrackPrimeDelaySeconds = 0.5;
	const int32 MaxSubscribeAttempts = 5;
	const double SubscribeRetryDelaySeconds = 0.5;

	TSharedPtr<FMoqNetworkTestState> State = MakeShared<FMoqNetworkTestState>();
	State->RelayUrl = Config.RelayUrl;
	State->Namespace = FString::Printf(TEXT("%s-%s"), *Config.NamespacePrefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	State->TrackName = FString::Printf(TEXT("%s-%s"), *Config.TrackPrefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(12));
	State->ExpectedText = FString::Printf(TEXT("Blueprint-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	State->ExpectedBinary = { 0xFE, 0xED, 0xFA, 0xCE, 0x1D };

	const auto MarkFailure = [State](const FString& Reason)
	{
		State->bEncounteredFailure = true;
		State->FailureReason = Reason;
	};

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

	auto StartBlueprintConnect = [this, State, ConnectTimeoutSeconds](UMoqClient* Client, bool bIsPublisher)
	{
		if (!Client)
		{
			AddError(TEXT("Connect async action received null client"));
			return false;
		}

		UMoqConnectClientAsyncAction* Action = UMoqConnectClientAsyncAction::ConnectClient(nullptr, Client, State->RelayUrl, ConnectTimeoutSeconds);
		if (!Action)
		{
			AddError(TEXT("Failed to allocate UMoqConnectClientAsyncAction"));
			return false;
		}

		UMoqAutomationEventSink* Sink = bIsPublisher ? State->PublisherSink.Get() : State->SubscriberSink.Get();
		if (Sink)
		{
			Action->OnConnected.AddDynamic(Sink, &UMoqAutomationEventSink::HandleConnectAsyncSuccess);
			Action->OnFailed.AddDynamic(Sink, &UMoqAutomationEventSink::HandleConnectAsyncFailure);
		}

		if (bIsPublisher)
		{
			State->PublisherConnectAction.Reset(Action);
		}
		else
		{
			State->SubscriberConnectAction.Reset(Action);
		}
		return true;
	};

	if (!StartBlueprintConnect(PublisherClientRaw, true) || !StartBlueprintConnect(SubscriberClientRaw, false))
	{
		return false;
	}

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bPublisherConnectActionCompleted && State->bSubscriberConnectActionCompleted;
		},
		ConnectTimeoutSeconds,
		this,
		State,
		TEXT("Blueprint connect actions")));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bPublisherConnected && State->bSubscriberConnected;
		},
		ConnectTimeoutSeconds,
		this,
		State,
		TEXT("MoQ clients reach connected state")));

	AddCommand(new FMoqLambdaLatentCommand([this, State, MarkFailure]()
	{
		UMoqClient* PublisherClient = State->PublisherClient.Get();
		if (!PublisherClient)
		{
			const FString Error = TEXT("Publisher client invalid before namespace announcement");
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		const FMoqResult AnnounceResult = PublisherClient->AnnounceNamespace(State->Namespace);
		if (!AnnounceResult.bSuccess)
		{
			const FString Error = FString::Printf(TEXT("AnnounceNamespace failed: %s"), *AnnounceResult.ErrorMessage);
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		UMoqPublisher* Publisher = PublisherClient->CreatePublisher(State->Namespace, State->TrackName, EMoqDeliveryMode::Stream);
		if (!TestNotNull(TEXT("Publisher created"), Publisher))
		{
			MarkFailure(TEXT("Publisher creation failed"));
			return;
		}
		State->Publisher.Reset(Publisher);

		const TArray<uint8> PrimePayload = { 0xAA, 0x01, 0xBB };
		const FMoqResult PrimeResult = Publisher->PublishData(PrimePayload, EMoqDeliveryMode::Stream);
		State->bTrackPrimed = PrimeResult.bSuccess;
		TestTrue(TEXT("Blueprint flow should prime the track"), PrimeResult.bSuccess);
	}));

	AddCommand(new FMoqDelayLatentCommand(TrackPrimeDelaySeconds));
	AddCommand(new FMoqLambdaLatentCommand([this, State]()
	{
		TestTrue(TEXT("Track primed"), State->bTrackPrimed);
	}));

	AddCommand(new FMoqLambdaLatentCommand([this, State, MarkFailure, MaxSubscribeAttempts, SubscribeRetryDelaySeconds]()
	{
		UMoqClient* SubscriberClient = State->SubscriberClient.Get();
		UMoqAutomationEventSink* Sink = State->SubscriberSink.Get();
		if (!SubscriberClient || !Sink)
		{
			const FString Error = TEXT("Subscriber client invalid before blueprint subscribe action");
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		UMoqSubscribeWithRetryAsyncAction* SubscribeAction = UMoqSubscribeWithRetryAsyncAction::SubscribeWithRetry(nullptr, SubscriberClient, State->Namespace, State->TrackName, MaxSubscribeAttempts, SubscribeRetryDelaySeconds);
		if (!SubscribeAction)
		{
			const FString Error = TEXT("Failed to allocate subscribe async action");
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		State->SubscribeAsyncAction.Reset(SubscribeAction);
		SubscribeAction->OnSubscribed.AddDynamic(Sink, &UMoqAutomationEventSink::HandleSubscribeAsyncSuccess);
		SubscribeAction->OnFailed.AddDynamic(Sink, &UMoqAutomationEventSink::HandleSubscribeAsyncFailure);
	}));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bBlueprintSubscriberReady;
		},
		PayloadTimeoutSeconds,
		this,
		State,
		TEXT("Blueprint subscriber creation")));

	AddCommand(new FMoqLambdaLatentCommand([this, State, MarkFailure]()
	{
		UMoqPublisher* Publisher = State->Publisher.Get();
		if (!Publisher)
		{
			const FString Error = TEXT("Publisher invalid before verification publishes");
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		const FMoqResult PublishTextResult = Publisher->PublishText(State->ExpectedText, EMoqDeliveryMode::Stream);
		if (!PublishTextResult.bSuccess)
		{
			const FString Error = FString::Printf(TEXT("PublishText failed: %s"), *PublishTextResult.ErrorMessage);
			AddError(Error);
			MarkFailure(Error);
			return;
		}

		const FMoqResult PublishBinaryResult = Publisher->PublishData(State->ExpectedBinary, EMoqDeliveryMode::Stream);
		if (!PublishBinaryResult.bSuccess)
		{
			const FString Error = FString::Printf(TEXT("PublishData failed: %s"), *PublishBinaryResult.ErrorMessage);
			AddError(Error);
			MarkFailure(Error);
		}
	}));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bTextReceived;
		},
		PayloadTimeoutSeconds,
		this,
		State,
		TEXT("Blueprint subscriber text payload")));

	AddCommand(new FMoqWaitConditionLatentCommand(
		[State]()
		{
			return State->bBinaryReceived;
		},
		PayloadTimeoutSeconds,
		this,
		State,
		TEXT("Blueprint subscriber binary payload")));

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

		State->Publisher.Reset();
		State->Subscriber.Reset();
		State->PublisherClient.Reset();
		State->SubscriberClient.Reset();
	}));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
