// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AgentDemo : ModuleRules
{
	public AgentDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayCameras",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// UE auto-adds <ModuleDir>/Public/, but subdirectories need explicit paths.
		PublicIncludePaths.AddRange(new string[] {
			"AgentDemo/Core/Public",
			"AgentDemo/Character/Public",
			"AgentDemo/Player/Public",
			"AgentDemo/SDK/Public",
			"AgentDemo/UI/Public"
		});

		// SDK 内部实现（Mock 服务等）仅供模块内引用
		PrivateIncludePaths.AddRange(new string[] {
			"AgentDemo/SDK/Private"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
