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

	mutable FAssassinsAttributeChangeSignature OnMoveSpeedChanged;

protected:

	//~UAttributeSet interface
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	//~End of UAttributeSet interface

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackDamage, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackDamage;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityPower, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AbilityPower;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Armor;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicResistance, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicResistance;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetrationPercent, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ArmorPenetrationPercent;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetrationFlat, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ArmorPenetrationFlat;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicPenetrationPercent, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicPenetrationPercent;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicPenetrationFlat, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicPenetrationFlat;

	// Me: The number of attack available per second
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackSpeed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = true))
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

private:

	UFUNCTION()
	void OnRep_AttackDamage(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AbilityPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MagicResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ArmorPenetrationFlat(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ArmorPenetrationPercent(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MagicPenetrationFlat(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MagicPenetrationPercent(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
};
