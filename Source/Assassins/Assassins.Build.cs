// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Assassins : ModuleRules
{
	public Assassins(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "Assassins"
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[] { 
                "Core", 
                "CoreUObject", 
                "Engine", 
                "InputCore", 
                "NavigationSystem", 
                "AIModule", 
                "Niagara", 
                "EnhancedInput",
                "ModularGameplay",
                "ModularGameplayActors",
                "GameplayTags",
                "GameplayTasks",
                "GameplayAbilities",
                "GameFeatures",
                "CommonLoadingScreen"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "UMG",
                "CommonUI",
                "CommonInput",
                "CommonGame",
                "CommonUser",
            }
        );
    }
}
