// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Attributes/AssassinsAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AssassinsHealthSet.generated.h"

struct FGameplayAttribute;

/**
 * Class that defines attributes that are necessary for taking damage.
 * Attribute examples include: health, shields, and resistances.
 */
UCLASS(BlueprintType)
class ASSASSINS_API UAssassinsHealthSet : public UAssassinsAttributeSet
{
	GENERATED_BODY()

public:

	UAssassinsHealthSet();

	ATTRIBUTE_ACCESSORS(UAssassinsHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UAssassinsHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UAssassinsHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(UAssassinsHealthSet, Damage);

	// Delegate when health changes due to damage/healing, some information may be missing on the client
	mutable FAssassinsAttributeEvent OnHealthChanged;

	// Delegate to broadcast when the health attribute reaches zero
	mutable FAssassinsAttributeEvent OnOutOfHealth;

protected:
	
	//~UAttributeSet interface
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	//~End of UAttributeSet interface

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// The current health attribute. The health will be capped by the max health attribute.
	// Me: It should be handled by damage or healing, not by itself (HideFromModifiers)
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	// The current max health attribute.  Max health is an attribute since gameplay effects can modify it.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	// Used to track when the health reaches 0.
	bool bOutOfHealth;

	// Store the health before any changes
	float HealthBeforeAttributeChange;

	// -------------------------------------------------------------------
	//	Meta Attribute (please keep attributes that aren't 'stateful' below 
	// -------------------------------------------------------------------

	// Incoming healing. This is mapped directly to +Health
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Healing;

	// Incoming damage. This is mapped directly to -Health
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Damage;
};
