// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "UObject/PrimaryAssetId.h"
#include "AssassinsDeveloperSettings.generated.h"

class UAssassinsPawnData;

/**
 * UAssassinsDeveloperSettings
 *
 * Settings for testing in the editor, kept per user(EditorPerProjectUserSettings), so that nobody's test setup reaches
 * the others. Found in Project Settings under the project name.
 *
 * They let Play In Editor start a match on a gameplay map right away, set up as if the players had hosted, joined and
 * picked their champions in the lobby. Play from the frontend map to go through the lobby instead.
 */
UCLASS(Config = EditorPerProjectUserSettings)
class ASSASSINS_API UAssassinsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UAssassinsDeveloperSettings();

	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface

	// PIE only: the experience to play in place of the one the map would use(its world settings, or the default).
	// The frontend map, which is also the lobby, keeps its own, and a match reached from the lobby keeps the one it was
	// hosted with(URL options win). Leave it empty to play what the map would.
	UPROPERTY(Config, EditAnywhere, Category = "Play In Editor", meta = (AllowedTypes = "AssassinsExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;

	// PIE only: the champions of the players, in the order they join. The host of a listen server comes first.
	// Players past the end, with an empty entry, or who already picked one in the lobby are left as they are.
	UPROPERTY(Config, EditAnywhere, Category = "Play In Editor")
	TArray<TSoftObjectPtr<UAssassinsPawnData>> ChampionOverrides;

	// PIE only: the bots to add, when the match was not told how many(the lobby tells it with the NumBots option). They
	// pick their champions themselves.
	UPROPERTY(Config, EditAnywhere, Category = "Play In Editor", meta = (ClampMin = 0))
	int32 NumBotsOverride = 0;
};
