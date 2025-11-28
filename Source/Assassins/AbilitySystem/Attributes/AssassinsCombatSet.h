// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Attributes/AssassinsAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AssassinsCombatSet.generated.h"

/**
 * Class that defines attributes that are necessary for applying damage or healing.
 * Attribute examples include: damage, healing, attack power and shield penetrations.
 */
UCLASS(BlueprintType)
class UAssassinsCombatSet : public UAssassinsAttributeSet
{
	GENERATED_BODY()
	
public:
	UAssassinsCombatSet();

	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, BaseHeal);

private:

	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseDamage;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseHeal;
};
