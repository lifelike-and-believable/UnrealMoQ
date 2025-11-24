// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMoQ.h"
#include "Modules/ModuleManager.h"
#include "moq_ffi.h"

#define LOCTEXT_NAMESPACE "FUnrealMoQModule"

void FUnrealMoQModule::StartupModule()
{
	if (moq_init())
	{
		UE_LOG(LogTemp, Log, TEXT("UnrealMoQ: moq_ffi initialized (version: %s)"), UTF8_TO_TCHAR(moq_version()));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealMoQ: Failed to initialize moq_ffi"));
	}
}

void FUnrealMoQModule::ShutdownModule()
{
	// Statically linked moq_ffi does not require explicit shutdown work here.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealMoQModule, UnrealMoQ)
