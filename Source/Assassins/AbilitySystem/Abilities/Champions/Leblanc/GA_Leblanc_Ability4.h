#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "GA_Leblanc_Ability4.generated.h"

UCLASS(Abstract)
class UGA_Leblanc_Ability4 : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Ability4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
