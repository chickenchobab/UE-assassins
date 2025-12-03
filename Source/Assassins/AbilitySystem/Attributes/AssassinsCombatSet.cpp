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

void UAssassinsCombatSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	// Me: TODO: What is the original instigator which started the whole chain?
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer = EffectContext.GetEffectCauser();

	if (GetMoveSpeed() != MoveSpeedBeforeAttributeChanged)
	{
		OnMoveSpeedChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MoveSpeedBeforeAttributeChanged, GetMoveSpeed());
	}
}
