#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "Ability_Leblanc_R.generated.h"

UCLASS(Abstract)
class UAbility_Leblanc_R : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UAbility_Leblanc_R(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:

	void WaitForRecast();

	UFUNCTION()
	void OnMimicStarted();

	UFUNCTION()
	void OnMimicEnded();

	UFUNCTION()
	void OnRecastReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastPressed(float TimeWaited);
};
