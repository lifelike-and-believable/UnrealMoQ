// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMoQ.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <windows.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "moq_ffi.h"

#define LOCTEXT_NAMESPACE "FUnrealMoQModule"

void FUnrealMoQModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// Get the base directory of this plugin
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("UnrealMoQ");
	if (!Plugin.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealMoQ: Failed to find plugin"));
		return;
	}
	FString BaseDir = Plugin->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString MoqFFILibraryPath;
	
#if PLATFORM_WINDOWS
	MoqFFILibraryPath = FPaths::Combine(*BaseDir, TEXT("ThirdParty/moq-ffi/moq_ffi/target/release/moq_ffi.dll"));
#elif PLATFORM_LINUX
	MoqFFILibraryPath = FPaths::Combine(*BaseDir, TEXT("ThirdParty/moq-ffi/moq_ffi/target/release/libmoq_ffi.so"));
#elif PLATFORM_MAC
	MoqFFILibraryPath = FPaths::Combine(*BaseDir, TEXT("ThirdParty/moq-ffi/moq_ffi/target/release/libmoq_ffi.dylib"));
#endif

	MoqFFIDllHandle = !MoqFFILibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*MoqFFILibraryPath) : nullptr;

	if (MoqFFIDllHandle)
	{
		// Initialize the MoQ FFI library
		if (moq_init())
		{
			UE_LOG(LogTemp, Log, TEXT("UnrealMoQ: Successfully loaded and initialized moq_ffi library from %s"), *MoqFFILibraryPath);
			UE_LOG(LogTemp, Log, TEXT("UnrealMoQ: Library version: %s"), UTF8_TO_TCHAR(moq_version()));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealMoQ: Loaded moq_ffi library but initialization failed"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealMoQ: Failed to load moq_ffi library from %s"), *MoqFFILibraryPath);
	}
}

void FUnrealMoQModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module. For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	// Free the dll handle
	if (MoqFFIDllHandle)
	{
		FPlatformProcess::FreeDllHandle(MoqFFIDllHandle);
		MoqFFIDllHandle = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealMoQModule, UnrealMoQ)
