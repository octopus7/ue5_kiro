// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class kiro_topdown : ModuleRules
{
	public kiro_topdown(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "NavigationSystem", "AIModule", "Sockets", "Networking", "UMG", "Slate", "SlateCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		
		// Add include paths for generated protobuf files
		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Generated"));
		
		// Disable specific warnings for protobuf compatibility
		bEnableUndefinedIdentifierWarnings = false;

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
