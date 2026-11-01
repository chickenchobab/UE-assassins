#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "Ability_LocationTargeted.generated.h"

class UAnimMontage;

UCLASS(Abstract)
class UAbility_LocationTargeted : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UAbility_LocationTargeted(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EvaluateCursorTransform(const FGameplayEventData& EventData);

	virtual void PlayMontage();
	virtual void PrePlayMontage();
	virtual void PostPlayMontage() {}
	virtual void OnMontageComplete();
	virtual void OnMontageCancelled();

private:

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double AbilityRange = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double EffectRadius = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector CursorLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector CursorLocationClamped = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FRotator CursorLookAtRotation = FRotator::ZeroRotator;
};
