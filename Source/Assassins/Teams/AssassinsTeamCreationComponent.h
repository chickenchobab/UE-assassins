// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"

#include "AssassinsTeamCreationComponent.generated.h"

class UAssassinsExperienceDefinition;
class UAssassinsTeamAsset;

UCLASS(Blueprintable)
class UAssassinsTeamCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()
	
public:
	UAssassinsTeamCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

private:
	void OnExperienceLoaded(const UAssassinsExperienceDefinition* Experience);

protected:
	// List of teams to create (id to display asset mapping)
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TMap<uint8, TObjectPtr<UAssassinsTeamAsset>> TeamsToCreate;

#if WITH_SERVER_CODE

protected:
	virtual void ServerCreateTeams();

private:

	void GetTeamBaseTransform(FTransform& BaseTransform, int32 TeamId, const TArray<AActor*>& TeamBases, bool& FoundTeamBase);

	void ServerCreateTeam(int32 TeamId, UAssassinsTeamAsset* TeamAsset, const FTransform& TeamBaseTransform);
#endif
};
