#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GA_Zed_ImitatedAbility.generated.h"

class AAssassinsZedShadow;

UCLASS(Abstract)
class UGA_Zed_ImitatedAbility : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Zed_ImitatedAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnMontageComplete() override;
	virtual void OnMontageCancelled() override;

	virtual FGameplayTag GetComboTag() const PURE_VIRTUAL(UGA_Zed_ImitatedAbility::GetComboTag, return FGameplayTag(););
	virtual FGameplayTag GetPartnerComboTag() const PURE_VIRTUAL(UGA_Zed_ImitatedAbility::GetPartnerComboTag, return FGameplayTag(););

	virtual void ImitateWith(AAssassinsZedShadow& Shadow) PURE_VIRTUAL(UGA_Zed_ImitatedAbility::ImitateWith, );

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
