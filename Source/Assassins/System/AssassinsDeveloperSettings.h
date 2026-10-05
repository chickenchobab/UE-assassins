#pragma once

#include "Engine/DeveloperSettings.h"
#include "UObject/PrimaryAssetId.h"
#include "AssassinsDeveloperSettings.generated.h"

class UAssassinsPawnData;

UCLASS(Config = EditorPerProjectUserSettings)
class ASSASSINS_API UAssassinsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UAssassinsDeveloperSettings();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category = "Play In Editor", meta = (AllowedTypes = "AssassinsExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;

	UPROPERTY(Config, EditAnywhere, Category = "Play In Editor")
	TArray<TSoftObjectPtr<UAssassinsPawnData>> ChampionOverrides;

	UPROPERTY(Config, EditAnywhere, Category = "Play In Editor", meta = (ClampMin = 0))
	int32 NumBotsOverride = 0;
};
