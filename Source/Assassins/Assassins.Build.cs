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
                "ModularGameplayActors"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CommonUI",
                "CommonInput",
                "CommonGame",
                "CommonUser",
            }
        );
    }
}
