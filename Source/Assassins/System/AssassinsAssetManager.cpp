// Fill out your copyright notice in the Description page of Project Settings.

#include "AssassinsAssetManager.h"
#include "AssassinsLogCategories.h"
#include "System/AssassinsGameData.h"
#include "Character/AssassinsPawnData.h"

const FName FAssassinsBundles::Equipped("Equipped");

UAssassinsAssetManager::UAssassinsAssetManager()
{
	DefaultPawnData = nullptr;
}

UAssassinsAssetManager& UAssassinsAssetManager::Get()
{
	check(GEngine);

	if (UAssassinsAssetManager* Singleton = Cast<UAssassinsAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogAssassins, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini. It must be set to AssassinsAssetManager!"));

	// Fatal error above prevents this form being called.
	return *NewObject<UAssassinsAssetManager>();
}

const UAssassinsGameData& UAssassinsAssetManager::GetGameData()
{
	if (LoadedGameData)
	{
		return *LoadedGameData;
	}
	return *LoadGameData(AssassinsGameDataPath, UAssassinsGameData::StaticClass()->GetFName());
}

const UAssassinsPawnData* UAssassinsAssetManager::GetDefaultPawnData() const
{
	return GetAsset(DefaultPawnData);
}

UObject* UAssassinsAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		if (UAssetManager::IsInitialized())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

void UAssassinsAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

UAssassinsGameData* UAssassinsAssetManager::LoadGameData(const TSoftObjectPtr<UAssassinsGameData>& DataPath, FPrimaryAssetType PrimaryAssetType)
{
	UAssassinsGameData* GameData = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);
	if (!DataPath.IsNull())
	{
		UE_LOG(LogAssassins, Log, TEXT("Loading GameData: %s ..."), *DataPath.ToString());
	}
	SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

	// This can be called recursively in the editor because it is called on demand from PostLoad so force a sync load for primary asset and async load the rest in that case
	if (GIsEditor)
	{
		GameData = DataPath.LoadSynchronous();
		LoadPrimaryAssetsWithType(PrimaryAssetType);
	}
	else
	{
		TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
		if (Handle.IsValid())
		{
			Handle->WaitUntilComplete(0.0f, false);

			// This should always work
			GameData = Cast<UAssassinsGameData>(Handle->GetLoadedAsset());
		}
	}

	if (GameData)
	{
		LoadedGameData = GameData;
	}
	else
	{
		// It is not acceptable to fail to load any GameData asset. It will result in soft failures that are hard to diagnose.
		UE_LOG(LogAssassins, Fatal, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."), *DataPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}

	return GameData;
}
