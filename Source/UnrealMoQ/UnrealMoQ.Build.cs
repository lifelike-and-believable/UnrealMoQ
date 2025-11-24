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
		string MoqFFIPath = Path.Combine(ThirdPartyPath, "moq-ffi", "moq_ffi");
		string MoqFFIIncludePath = Path.Combine(MoqFFIPath, "include");
		
		PublicIncludePaths.Add(MoqFFIIncludePath);
		
		// Platform-specific library paths and linking
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string LibPath = Path.Combine(MoqFFIPath, "target", "release");
			string BinPath = LibPath;
			
			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(LibPath, "moq_ffi.dll.lib"));
			
			// Delay-load the DLL so we can control when it's loaded
			PublicDelayLoadDLLs.Add("moq_ffi.dll");
			
			// Stage the DLL to the binaries directory
			RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "moq_ffi.dll"), Path.Combine(BinPath, "moq_ffi.dll"));
			
			// Also stage the PDB for debugging
			string PdbPath = Path.Combine(BinPath, "moq_ffi.pdb");
			if (File.Exists(PdbPath))
			{
				RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "moq_ffi.pdb"), PdbPath);
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string LibPath = Path.Combine(MoqFFIPath, "target", "release");
			
			// Link the shared library
			PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libmoq_ffi.so"));
			
			// Stage the shared library
			RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "libmoq_ffi.so"), Path.Combine(LibPath, "libmoq_ffi.so"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string LibPath = Path.Combine(MoqFFIPath, "target", "release");
			
			// Link the dynamic library
			PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libmoq_ffi.dylib"));
			
			// Stage the dynamic library
			RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "libmoq_ffi.dylib"), Path.Combine(LibPath, "libmoq_ffi.dylib"));
		}
	}
}
