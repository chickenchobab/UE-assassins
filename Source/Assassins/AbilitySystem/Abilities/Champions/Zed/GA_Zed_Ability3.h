// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_ImitatedAbility.h"
#include "Engine/EngineTypes.h"
#include "GA_Zed_Ability3.generated.h"

class UGameplayEffect;

/**
 * UGA_Zed_Ability3
 *
 * Zed E, Shadow Slash. Zed slashes around him, and each of his shadows slashes along, once they are all there
 * (UGA_Zed_ImitatedAbility); every unit is hit once. Was the GA_Zed_Ability3 blueprint.
 */
UCLASS(Abstract)
class UGA_Zed_Ability3 : public UGA_Zed_ImitatedAbility
{
	GENERATED_BODY()

public:

	UGA_Zed_Ability3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGA_LocationTargeted_Immediate interface
	virtual void EvaluateCursorTransform(const FGameplayEventData& EventData) override {}
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	//~End of UGA_LocationTargeted_Immediate interface

	//~UGA_Zed_ImitatedAbility interface
	virtual FGameplayTag GetComboTag() const override;
	virtual FGameplayTag GetPartnerComboTag() const override;
	virtual void ImitateWith(AAssassinsZedShadow& Shadow) override;
	//~End of UGA_Zed_ImitatedAbility interface

protected:

	// What the slashes hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TEnumAsByte<EObjectTypeQuery>> TypesToSphereTrace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Marks the units a slash hit, so that the next slashes leave them alone.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> SlashTargetEffectClass;

private:

	// Slashes around the source, Zed or a shadow.
	void SlashAround(AActor* SourceActor);

	// Hits the enemy, unless a slash did already.
	void ApplyDamageAndStatus(AActor* HitActor);
};
