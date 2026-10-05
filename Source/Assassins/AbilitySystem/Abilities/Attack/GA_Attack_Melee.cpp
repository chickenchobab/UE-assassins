// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"

void UGA_Attack_Melee::OnAttackHit()
{
	// The target takes the damage right away: there is nothing travelling to it.
	// It may have died during the swing, and applying to an actor without an ability system fails a check.
	AttackDamageSpecHandle = MakeEffectSpecHandle(GetAttackEffectClass());
	if (IsValid(AbilityTargetActor))
	{
		ApplyGameplayEffectSpecToTargetActor(AttackDamageSpecHandle, AbilityTargetActor);
	}

	// The one who clicked is the one asking for the attack after this one.
	SendActivateAttackEvent(AbilityTargetActor);

	Super::OnAttackHit();

	// The montage was already over when the hit landed, so the attack has nothing left to wait for.
	if (IsMontageEnded)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
	}
}
