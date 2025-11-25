// Copyright Epic Games, Inc. All Rights Reserved.

#include "MoqAutomationBlueprintLibrary.h"

#include "Containers/Ticker.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopeExit.h"
#include "Math/UnrealMathUtility.h"
#include "Async/TaskGraphInterfaces.h"

void UMoqAutomationBlueprintLibrary::PumpMoqEventLoop(int32 Iterations)
{
	const int32 SafeIterations = FMath::Clamp(Iterations, 1, 256);

	for (int32 Index = 0; Index < SafeIterations; ++Index)
	{
		FTSTicker::GetCoreTicker().Tick(0.f);
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FPlatformProcess::SleepNoStats(0.0f);
	}
}
