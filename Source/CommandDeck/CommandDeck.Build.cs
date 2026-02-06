// Copyright Loxton Enterprises, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CommandDeck : ModuleRules
{
	public CommandDeck(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Json"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CommandDeckCore",
				"DeveloperSettings",
				"EngineSettings",
				"InputCore",
				"JsonUtilities",
				"Slate",
				"SlateCore",
				"Projects",
				"WebSockets"
			});

		OptimizeCode = CodeOptimization.Default;

		if (Target.bBuildEditor == true)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"UnrealEd"
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"LevelEditor"
				}
			);
		}
	}
}
