// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsCharacter.h"
#include "GameplayTagContainer.h"
#include "AssassinsChampion.generated.h"

class UAssassinsCameraComponent;
class UAssassinsChampionSkillState;
class UAssassinsHealthComponent;

/**
 * AAssassinsChampion
 *
 * Base class of the champions, formerly the B_ChampionBase blueprint. Every champion blueprint(B_Zed, ...) derives from
 * it directly: what a champion's abilities share goes to its skill state, whose class its ability set names. Its mesh is
 * the blueprint's to set up, scale included.
 * Has the camera of the player, and keeps the aggro tags of the champion's attackers up to date, which turrets and
 * minions use to pick their target.
 */
UCLASS(Abstract)
class ASSASSINS_API AAssassinsChampion : public AAssassinsCharacter
{
	GENERATED_BODY()

public:

	AAssassinsChampion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// What the champion's abilities share. Null while the pawn data is unknown, and for a champion whose ability sets
	// name no skill state.
	UAssassinsChampionSkillState* GetSkillState();

	template <class T>
	T* GetSkillState() { return Cast<T>(GetSkillState()); }

	// The skill state of the actor, when it is a champion whose skill state is a T.
	template <class T>
	static T* FindSkillState(AActor* Actor)
	{
		AAssassinsChampion* Champion = Cast<AAssassinsChampion>(Actor);
		return Champion ? Champion->GetSkillState<T>() : nullptr;
	}

	UAssassinsCameraComponent* GetAssassinsCameraComponent() const { return CameraComponent; }

protected:

	//~AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

	//~AAssassinsCharacter interface
	virtual void OnAbilitySystemInitialized() override;
	virtual void HandleGenericGameplayTagEvent_Implementation(const FGameplayTag Tag, int32 NewCount) override;
	//~End of AAssassinsCharacter interface

private:

	// Gives the aggro tag to whoever damaged the champion. A heal draws no aggro.
	UFUNCTION()
	void GrantAggroTagToInstigator(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* HealthInstigator);

	void HandleAggroTagExpired();

	// Makes the skill state out of the pawn data, once it is known. The ability system is ready for the pawn right after
	// the pawn data on every machine, which is when it is made; anyone asking before gets it made then.
	void CreateSkillState();

	FTimerHandle AggroTagTimerHandle;

	// The top down camera of the player playing the champion. Was made for every character, minions included.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsCameraComponent> CameraComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAssassinsChampionSkillState> SkillState;

	// Whether the pawn data was read, with or without a skill state in it.
	bool bSkillStateResolved = false;
};
