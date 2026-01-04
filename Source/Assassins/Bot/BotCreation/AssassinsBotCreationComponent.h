// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"
#include "AssassinsBotCreationComponent.generated.h"

class UAssassinsExperienceDefinition;
class AAIController;

UCLASS()
class UAssassinsBotCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()
	
public:
	UAssassinsBotCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

private:
	void OnExperienceLoaded(const UAssassinsExperienceDefinition* Experience);

protected:

	/** Spawns bots up to NumBotsToCreate */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = Gameplay)
	void ServerCreateBots();

	/** Always creates a single bot */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Gameplay)
	virtual void SpawnOneBot();

	/** Deletes the last created bot if possible */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Gameplay)
	virtual void RemoveOneBot(AAIController* Controller);

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	float InitialSpawnDelay;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	float SpawnDelay;

};
