// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Teams/AssassinsTeamAgentInterface.h"
#include "AssassinsTeamBaseActor.generated.h"

class UAssassinsAbilitySystemComponent;
class UAssassinsAbilitySet;
class UAssassinsCombatSet;
class UAssassinsHealthComponent;
class UAssassinsHealthSet;
class USkeletalMeshComponent;
class USphereComponent;
class UAnimMontage;

/**
 * Actor indicating the base(nexus) of a team. Minion waves are spawned here
 * and get the transform of this actor initially.
 * 
 * Me: The base owns a self-contained ability system so that it can be damaged
 * through the same gameplay effect path as characters(AssassinsDamageExecution).
 */
UCLASS()
class AAssassinsTeamBaseActor 
	: public AActor
	, public IAbilitySystemInterface
	, public IAssassinsTeamAgentInterface
{
	GENERATED_BODY()
	
public:

	AAssassinsTeamBaseActor();

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~IAbilitySystemInterface interface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface interface

	//~IAssassinsTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~End of IAssassinsTeamAgentInterface interface

	// Me: Minions would spawn inside the base mesh if the actor transform was used directly.
	UFUNCTION(BlueprintPure, Category = Teams)
	FTransform GetMinionSpawnTransform() const;

	// Me: Ends the online session and sends every player back to the frontend main menu.
	// Authority only, since the base is destroyed on the server.
	UFUNCTION(BlueprintCallable, Category = Teams)
	void ReturnAllPlayersToFrontend(float DelaySeconds = 0.0f);

	UPROPERTY(EditAnywhere, Category = Teams)
	int32 TeamId;

	// Played by the death ability when the base is destroyed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Base")
	TObjectPtr<UAnimMontage> DeadMontage;

protected:

	UFUNCTION()
	virtual void HandleDeathStarted();
	UFUNCTION()
	virtual void HandleDeathFinished();

	void DoReturnAllPlayersToFrontend();

	// Me: Grants the attribute sets(health, armor and magic resistance) and the death ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsAbilitySet> AbilitySet;

	// Me: Added to the ability system on initialization, the same way AAssassinsCharacter does with
	// CharacterOwnedTags. Structure.Nexus goes here so abilities know what they are looking at.
	UPROPERTY(EditDefaultsOnly, Category = "Assassins|Base")
	FGameplayTagContainer BaseOwnedTags;

private:

	UPROPERTY(VisibleAnywhere, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsAbilitySystemComponent> AbilitySystemComponent;

	// These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
	UPROPERTY()
	TObjectPtr<const UAssassinsHealthSet> HealthSet;
	UPROPERTY()
	TObjectPtr<const UAssassinsCombatSet> CombatSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Health", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsHealthComponent> HealthComponent;

	// Me: The root of the base. Its own object channel is what lets a single volume block pawns
	// (a pawn typed volume can't, since character capsules overlap the pawn channel), take
	// projectile hits and answer the cursor targeting trace at the same time.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Collision", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionVolume;

	// Me: Purely visual. All collision belongs to the volume above, so no physics asset is needed.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Base", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Teams, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> MinionSpawnPoint;

	FTimerHandle ReturnToFrontendTimerHandle;
};
