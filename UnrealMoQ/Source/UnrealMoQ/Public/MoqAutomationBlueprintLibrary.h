// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MoqAutomationBlueprintLibrary.generated.h"

/** Utility helpers exposed for automation and blueprint-driven integration testing. */
UCLASS()
class UNREALMOQ_API UMoqAutomationBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Pumps the core ticker/task graph for a short interval so async callbacks (connect/subscriber events)
	 * can fire while running blueprint-driven automation.
	 */
	UFUNCTION(BlueprintCallable, Category = "MoQ|Automation", meta = (AdvancedDisplay = "Iterations"))
	static void PumpMoqEventLoop(int32 Iterations = 4);
};
