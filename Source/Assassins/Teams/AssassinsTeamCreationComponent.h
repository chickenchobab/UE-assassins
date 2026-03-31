// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"

#include "AssassinsTeamCreationComponent.generated.h"

class UAssassinsExperienceDefinition;
class UAssassinsTeamAsset;
class AAssassinsPlayerState;

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
	virtual void ServerAssignPlayersToTeams();

	/** Sets the team ID for the given player state. */
	virtual void ServerChooseTeamForPlayer(AAssassinsPlayerState* PS);


private:

	void ServerCreateTeam(int32 TeamId, UAssassinsTeamAsset* TeamAsset, const FTransform& TeamBaseTransform);
	void OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer);

	void GetTeamBaseTransform(FTransform& BaseTransform, int32 TeamId, const TArray<AActor*>& TeamBases, bool& FoundTeamBase);

	int32 GetLeastPopulatedTeamID() const;

#endif
};
