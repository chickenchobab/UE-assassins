// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"
#include "AssassinsPlayerSpawnerComponent.generated.h"

class APlayerStart;
class AAssassinsPlayerStart;

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsPlayerSpawnerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UAssassinsPlayerSpawnerComponent(const FObjectInitializer& ObjectInitializer);

	//~AActorComponent interface
	virtual void InitializeComponent() override;
	//~End of AActorComponent interface

protected:
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) {}
	
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName=OnFinishRestartPlayer))
	void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);
	
private:
	/** We proxy these calls from AAssassinsGameMode to this component so that each experience can more easily customize the respawn system they want. */
	bool ControllerCanRestart(AController* Player);
	AActor* ChoosePlayerStart(AController* Player);
	void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation);
	friend class AAssassinsGameMode;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAssassinsPlayerStart>> CachedPlayerStarts;

private:
	void OnLevelLoaded(ULevel* InLevel, UWorld* InWorld);
	void HandleOnActorSpawned(AActor* SpawnedActor);

#if WITH_EDITOR
	APlayerStart* FindPlayFromHereStart(AController* Player);
#endif
};
