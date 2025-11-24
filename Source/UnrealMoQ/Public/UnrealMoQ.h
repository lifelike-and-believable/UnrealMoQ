// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUnrealMoQModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** 
	 * Handle to the loaded moq_ffi DLL/SO/DYLIB
	 * Initialized to nullptr on construction
	 * Loaded during StartupModule and freed during ShutdownModule
	 */
	void* MoqFFIDllHandle;
};
