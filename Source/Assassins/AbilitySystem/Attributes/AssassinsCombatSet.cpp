// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

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

void UAssassinsCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, AttackDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, AbilityPower, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, Armor, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, MagicResistance, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, ArmorPenetrationFlat, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, ArmorPenetrationPercent, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, MagicPenetrationFlat, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, MagicPenetrationPercent, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, AttackSpeed, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAssassinsCombatSet, MoveSpeed, COND_OwnerOnly, REPNOTIFY_Always);
}

void UAssassinsCombatSet::OnRep_AttackDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, AttackDamage, OldValue);
}

void UAssassinsCombatSet::OnRep_AbilityPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, AbilityPower, OldValue);
}

void UAssassinsCombatSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, Armor, OldValue);
}

void UAssassinsCombatSet::OnRep_MagicResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, MagicResistance, OldValue);
}

void UAssassinsCombatSet::OnRep_ArmorPenetrationFlat(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, ArmorPenetrationFlat, OldValue);
}

void UAssassinsCombatSet::OnRep_ArmorPenetrationPercent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, ArmorPenetrationPercent, OldValue);
}

void UAssassinsCombatSet::OnRep_MagicPenetrationFlat(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, MagicPenetrationFlat, OldValue);
}

void UAssassinsCombatSet::OnRep_MagicPenetrationPercent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, MagicPenetrationPercent, OldValue);
}

void UAssassinsCombatSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, AttackSpeed, OldValue);
}

void UAssassinsCombatSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAssassinsCombatSet, MoveSpeed, OldValue);
}