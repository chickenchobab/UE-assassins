// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bot/BotCreation/AssassinsBotCreationComponent.h"
#include "AssassinsPlayerBotCreationComponent.generated.h"

class AAssassinsCharacter;
class UAssassinsPawnData;

UCLASS(Blueprintable, Abstract)
class UAssassinsPlayerBotCreationComponent : public UAssassinsBotCreationComponent
{
	GENERATED_BODY()

protected:

#if WITH_SERVER_CODE
	//~UAssassinsBotCreationComponent interface
	virtual void ServerCreateBots() override;
	//~End of UAssassinsBotCreationComponent interface

	void SpawnOneBot(UAssassinsPawnData* PawnData, int32 TeamId);
#endif

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay)
	TMap <TObjectPtr< UAssassinsPawnData > , int32 > BotsToCreate;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AAIController>> SpawnedBotList;
};
