// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "GameplayEffectExtension.h"

UAssassinsCombatSet::UAssassinsCombatSet()
	: AttackDamage(0.0f)
	, AbilityPower(0.0f)
	, Armor(0.0f)
	, MagicResistance(0.0f)
	, ArmorPenetrationPercent(0.0f)
	, ArmorPenetrationFlat(0.0f)
	, MagicPenetrationPercent(0.0f)
	, MagicPenetrationFlat(0.0f)
	, AttackSpeed(0.0f)
	, MoveSpeed(0.0f)
    , PhysicalDamage(0.0f)
    , MagicDamage(0.0f)
    , TrueDamage(0.0f)
{
}

bool UAssassinsCombatSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	MoveSpeedBeforeAttributeChanged = GetMoveSpeed();

	return true;
}

void UAssassinsCombatSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	NewValue = FMath::Max(0.0f, NewValue);
}

void UAssassinsCombatSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute() && GetMoveSpeed() != MoveSpeedBeforeAttributeChanged)
	{
		// Me: Notify character so its movement component can apply the update
		OnMoveSpeedChanged.Broadcast(MoveSpeedBeforeAttributeChanged, GetMoveSpeed());
	}
}
