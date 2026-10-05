// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class AssassinsTests : ModuleRules
{
	public AssassinsTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// The game headers are included from the game module's folder(Character/..., Player/...), as the game module
		// does itself.
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "Assassins"));

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"AIModule",
				"DeveloperSettings",
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks",
				"ModularGameplay",
				"ModularGameplayActors",
				"CommonGame",
				"Niagara",
				"Assassins"
			}
		);
	}
}
