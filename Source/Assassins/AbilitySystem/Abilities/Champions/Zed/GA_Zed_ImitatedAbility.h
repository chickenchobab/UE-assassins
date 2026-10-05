// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GA_Zed_ImitatedAbility.generated.h"

class AAssassinsZedShadow;

/**
 * UGA_Zed_ImitatedAbility
 *
 * What Zed's Q(Ability1) and E(Ability3) share: each of his shadows does what he does, once they are all there. The
 * shadow of Ability2 on its way arrives first, and of Q and E cast together the second waits for the first to be done
 * with the shadows(the combo tags, on the owning side). The owning client decides when and tells the server, which has
 * the shadows imitate in the window of the client's prediction key. The ability ends once its montage and the shadows
 * are both done. Was written twice, in GA_Zed_Ability1 and GA_Zed_Ability3.
 */
UCLASS(Abstract)
class UGA_Zed_ImitatedAbility : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Zed_ImitatedAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_LocationTargeted_Immediate interface
	virtual void OnMontageComplete() override;
	virtual void OnMontageCancelled() override;
	//~End of UGA_LocationTargeted_Immediate interface

	// The combo tag of this ability, and the one of the other ability the shadows imitate.
	virtual FGameplayTag GetComboTag() const PURE_VIRTUAL(UGA_Zed_ImitatedAbility::GetComboTag, return FGameplayTag(););
	virtual FGameplayTag GetPartnerComboTag() const PURE_VIRTUAL(UGA_Zed_ImitatedAbility::GetPartnerComboTag, return FGameplayTag(););

	// What a shadow does to imitate the ability, on the owning client and on the server.
	virtual void ImitateWith(AAssassinsZedShadow& Shadow) PURE_VIRTUAL(UGA_Zed_ImitatedAbility::ImitateWith, );

	// Owning side, as the cast starts: the combo tag goes on, unless the other ability came first and has its own on.
	void BeginCombo();

	// Once the montage plays: has the shadows imitate when they are all there.
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
