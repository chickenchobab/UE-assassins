// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/AssetManager.h"
#include "AssassinsAssetManager.generated.h"

class UAssassinsPawnData;

struct FAssassinsBundles
{
	static const FName Equipped;
};

/**
* Game implementation of the asset manager that overrides functionality and stores game - specific types.
* It is expected that most games will want to override AssetManager as it provides a good place for game - specific loading logic.
* This class is used by setting 'AssetManagerClassName' in DefaultEngine.ini.
*/
UCLASS(Config = Game)
class ASSASSINS_API UAssassinsAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	UAssassinsAssetManager();

	// Returns the AssetManager singleton object.
	static UAssassinsAssetManager& Get();

	// Return the asset referenced by a TSoftObjectPtr. This will synchronously load the asset if it's not already loaded.
	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	const UAssassinsPawnData* GetDefaultPawnData() const;

protected:
	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);

	// Thread safe way of adding a loaded asset to keep in memory.
	void AddLoadedAsset(const UObject* Asset);

protected:
	UPROPERTY(Config)
	TSoftObjectPtr<UAssassinsPawnData> DefaultPawnData;

private:
	// Assets loaded and tracked by the asset manager.
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	// Used for a scope lock when modifying the list of load assets.
	FCriticalSection LoadedAssetsCritical;
};

template<typename AssetType>
AssetType* UAssassinsAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
		ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
	}

	if (LoadedAsset && bKeepInMemory)
	{
		Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
	}

	return LoadedAsset;
}