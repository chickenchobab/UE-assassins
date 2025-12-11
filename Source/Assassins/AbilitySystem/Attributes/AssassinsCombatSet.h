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

	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, AttackDamage);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, AbilityPower);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, Armor);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, MagicResistance);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, ArmorPenetrationPercent);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, ArmorPenetrationFlat);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, MagicPenetrationPercent);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, MagicPenetrationFlat);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, AttackSpeed);
	ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, MoveSpeed);
    ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, PhysicalDamage);
    ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, MagicDamage);
    ATTRIBUTE_ACCESSORS(UAssassinsCombatSet, TrueDamage);

	mutable FAssassinsAttributeEvent OnMoveSpeedChanged;

protected:

	//~UAttributeSet interface
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	//~End of UAttributeSet interface

private:

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackDamage;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AbilityPower;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Armor;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicResistance;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ArmorPenetrationPercent;
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ArmorPenetrationFlat;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicPenetrationPercent;
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicPenetrationFlat;

	// Me: The number of attack available per second
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MoveSpeed;

	float MoveSpeedBeforeAttributeChanged;

    // -------------------------------------------------------------------------------------------------------
    //	Meta Attribute that aren't 'stateful' and are populated only by calculation modifiers before execution
    // -------------------------------------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true, HideFromModifiers))
    FGameplayAttributeData PhysicalDamage;

    UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true, HideFromModifiers))
    FGameplayAttributeData MagicDamage;

    UPROPERTY(BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true, HideFromModifiers))
    FGameplayAttributeData TrueDamage;
};
