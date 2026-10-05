#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility_Death.h"
#include "GA_Death.generated.h"

class USceneComponent;

UCLASS(Abstract)
class UGA_Death : public UAssassinsGameplayAbility_Death
{
	GENERATED_BODY()

public:

	UGA_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void PlayDeath();

	virtual void OnMontageEnd();

	UPROPERTY(EditDefaultsOnly, Category = "Assassins|Ability")
	TSubclassOf<USceneComponent> IndicatorComponentClass;

private:

	UFUNCTION()
	void HandleMontageEnded();

	bool bMontageEnded = false;
};
