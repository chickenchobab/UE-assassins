#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_LocationTargeted.h"
#include "Ability_Zed_ShadowImitable.generated.h"

class AZedShadow;

UCLASS(Abstract)
class UAbility_Zed_ShadowImitable : public UAbility_LocationTargeted
{
	GENERATED_BODY()

public:

	UAbility_Zed_ShadowImitable(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnMontageComplete() override;
	virtual void OnMontageCancelled() override;

	virtual FGameplayTag GetComboTag() const PURE_VIRTUAL(UAbility_Zed_ShadowImitable::GetComboTag, return FGameplayTag(););
	virtual FGameplayTag GetPartnerComboTag() const PURE_VIRTUAL(UAbility_Zed_ShadowImitable::GetPartnerComboTag, return FGameplayTag(););

	virtual void ImitateWith(AZedShadow& Shadow) PURE_VIRTUAL(UAbility_Zed_ShadowImitable::ImitateWith, );

	void BeginCombo();

	void WaitToImitate();

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool IsShadowHandled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool IsMontageEnded = false;

private:

	void MakeShadowsImitate();
	void HandleShadowsDone();

	UFUNCTION()
	void OnShadowSpawned();

	UFUNCTION()
	void OnComboPartnerDone();

	UFUNCTION()
	void OnImitateRequested();
};
