// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bot/BotCreation/AssassinsBotCreationComponent.h"
#include "Teams/AssassinsTeamAgentInterface.h"

#include "AssassinsMinionCreationComponent.generated.h"

class UAssassinsExperienceDefinition;
class AAIController;
class AAssassinsCharacterWithAbilities;

UENUM(BlueprintType)
enum class EAssassinsMinionWaveSpawningState : uint8
{
	WaitingSpawnNewWave,
	SpawningMinion,
	DelayBetweenMinion
};

UCLASS(Blueprintable, Abstract)
class UAssassinsMinionCreationComponent : public UAssassinsBotCreationComponent
{
	GENERATED_BODY()
	
public:
	UAssassinsMinionCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~End of UActorComponent interface
	
protected:

	//~UAssassinsBotCreationComponent interface
	virtual void ServerCreateBots_Implementation() override;
	virtual void SpawnOneBot() override;
	//~End of UAssassinsBotCreationComponent interface

	UFUNCTION(BlueprintImplementableEvent, Category = Gameplay)
	void ChangeTeam();

	UFUNCTION(BlueprintImplementableEvent, Category = Gameplay)
	void UpgradeMinion();

protected:

	//////////////////////////////////////////
	// Minion wave info
	//////////////////////////////////////////

	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	int32 MinionWaveSize;

	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	float MinionWaveTerm;

	// The number of melee or ranged minions
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	int32 NumNormalMinions;

	////////////////////////////////////////////////
	// Information of the current minion to spawn
	////////////////////////////////////////////////

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Gameplay)
	FGenericTeamId MinionTeamID;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Gameplay)
	TSubclassOf<AAssassinsCharacterWithAbilities> MinionClass;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Gameplay)
	FVector MinionSpawnLocation;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Gameplay)
	FRotator MinionSpawnRotation;

private:

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	EAssassinsMinionWaveSpawningState MinionSpawnState;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	int32 MinionSpawnCount;
};
 