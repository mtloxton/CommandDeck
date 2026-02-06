// Copyright Loxton Enterprises, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CommandDeckCore : ModuleRules
{
	public CommandDeckCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore",
				"Slate",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
			}
		);

		OptimizeCode = CodeOptimization.Default;
	}
}