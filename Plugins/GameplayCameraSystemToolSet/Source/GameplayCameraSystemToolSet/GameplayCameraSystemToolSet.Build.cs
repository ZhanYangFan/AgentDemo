// Copyright AgentDemo Project. All Rights Reserved.

using UnrealBuildTool;

public class GameplayCameraSystemToolSet : ModuleRules
{
	public GameplayCameraSystemToolSet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.Add("Core");

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"CoreUObject",
			"Engine",
			"GameplayCameras",
			"GameplayCamerasEditor",
			"ToolsetRegistry",
			"UnrealEd",
		});
	}
}
