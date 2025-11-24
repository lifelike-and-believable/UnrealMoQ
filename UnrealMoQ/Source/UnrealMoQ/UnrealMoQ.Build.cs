// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class UnrealMoQ : ModuleRules
{
	public UnrealMoQ(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
		);

		// Add moq-ffi third party library
		string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
		string ThirdPartyPath = Path.Combine(PluginPath, "ThirdParty");
		string MoqFFIPath = Path.Combine(ThirdPartyPath, "moq-ffi");
		string MoqFFIIncludePath = Path.Combine(MoqFFIPath, "include");
		string MoqFFILibRoot = Path.Combine(MoqFFIPath, "lib");
		
		PublicIncludePaths.Add(MoqFFIIncludePath);
		PublicDefinitions.Add("MOQ_FFI_STATIC=1");
		
		// Platform-specific library paths and linking
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string LibPath = Path.Combine(MoqFFILibRoot, "Win64");
			string StaticLib = Path.Combine(LibPath, "moq_ffi.lib");
			if (!File.Exists(StaticLib))
			{
				throw new BuildException($"Required moq-ffi library not found: {StaticLib}");
			}
			PublicAdditionalLibraries.Add(StaticLib);
			PublicSystemLibraries.AddRange(new string[]
			{
				"userenv.lib",
				"ntdll.lib"
			});
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string LibPath = Path.Combine(MoqFFILibRoot, "Linux");
			string StaticLib = Path.Combine(LibPath, "libmoq_ffi.a");
			if (!File.Exists(StaticLib))
			{
				throw new BuildException($"Required moq-ffi library not found: {StaticLib}");
			}
			PublicAdditionalLibraries.Add(StaticLib);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string LibPath = Path.Combine(MoqFFILibRoot, "Mac");
			string StaticLib = Path.Combine(LibPath, "libmoq_ffi.a");
			if (!File.Exists(StaticLib))
			{
				throw new BuildException($"Required moq-ffi library not found: {StaticLib}");
			}
			PublicAdditionalLibraries.Add(StaticLib);
		}
	}
}
