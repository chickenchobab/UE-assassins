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

	virtual void ServerCreateBots();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	float InitialSpawnDelay;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	float SpawnDelay;

};
