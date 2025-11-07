// Fill out your copyright notice in the Description page of Project Settings.

#include "AssassinsAssetManager.h"
#include "AssassinsLogCategories.h"
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

	UE_LOG(LogAssassins, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to AssassinsAssetManager!"));

	// Fatal error above prevents this form being called.
	return *NewObject<UAssassinsAssetManager>();
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
