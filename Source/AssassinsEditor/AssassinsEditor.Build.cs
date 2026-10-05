// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class AssassinsEditor : ModuleRules
{
	public AssassinsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				"AssassinsEditor"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"BlueprintGraph",
				"AssetRegistry",
				"Json"
			}
		);
	}
}
