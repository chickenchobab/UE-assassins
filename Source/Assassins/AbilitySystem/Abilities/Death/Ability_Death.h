#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "Ability_Death.generated.h"

class USceneComponent;

UCLASS(Abstract)
class UAbility_Death : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void StartDeath();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void FinishDeath();

	virtual void PlayDeath();

	virtual void OnMontageEnd();

	UPROPERTY(EditDefaultsOnly, Category = "Assassins|Ability")
	TSubclassOf<USceneComponent> IndicatorComponentClass;

private:

	UFUNCTION()
	void HandleMontageEnded();

	UPROPERTY(EditDefaultsOnly, Category = "Assassins|Ability")
	bool bCancelAllAbilities;

	bool bMontageEnded = false;
};
