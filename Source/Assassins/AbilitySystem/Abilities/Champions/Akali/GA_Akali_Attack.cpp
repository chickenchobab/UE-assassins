// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Attack.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "Character/AssassinsCharacter.h"
#include "NativeGameplayTags.h"

namespace AkaliAttackTags
{
	// Set while her passive is ready: the next attack is the empowered one.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_PASSIVE, "Status.Combo.Passive");

	// Set while her kick is running, so that the attack after it plays the montage that follows the kick.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ABILITY3, "Status.Ability.Ability3");
};

void UGA_Akali_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();

	// Only an attack asked for by an event can be the one following the kick.
	if (TriggerEventData != nullptr)
	{
		ShouldPlayAttackAfterKick = AvatarCharacter && AvatarCharacter->HasGameplayTag(AkaliAttackTags::TAG_STATUS_ABILITY_ABILITY3);
	}

	CheckAttackRange();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Akali_Attack::CheckAttackRange()
{
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter && AvatarCharacter->HasGameplayTag(AkaliAttackTags::TAG_STATUS_COMBO_PASSIVE))
	{
		AbilityRange = AbilityRange * 2.0;
	}
}

void UGA_Akali_Attack::OnAttackHit()
{
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter && AvatarCharacter->HasGameplayTag(AkaliAttackTags::TAG_STATUS_COMBO_PASSIVE))
	{
		// The empowered attack spends the passive: the extra damage lands and the tag goes away with the effect granting it.
		// The target may have died during the swing.
		if (IsValid(AbilityTargetActor))
		{
			ApplyGameplayEffectSpecToTargetActor(MakeEffectSpecHandle(PassiveDamageEffectClass), AbilityTargetActor);
		}

		FGameplayTagContainer PassiveTags;
		PassiveTags.AddTag(AkaliAttackTags::TAG_STATUS_COMBO_PASSIVE);
		BP_RemoveGameplayEffectFromOwnerWithGrantedTags(PassiveTags, /*StacksToRemove*/ -1);
	}

	Super::OnAttackHit();
}

void UGA_Akali_Attack::SetMontageToPlay()
{
	// The attack that follows the kick comes first: it is asked for while the passive may still be up.
	if (ShouldPlayAttackAfterKick && IsValid(AttackAfterKickMontageSet))
	{
		MontageToPlay = AttackAfterKickMontageSet->Montage;
		HitEventTime = AttackAfterKickMontageSet->Timing;
		AdjustMontageRateAndEventTime();
		return;
	}

	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter && AvatarCharacter->HasGameplayTag(AkaliAttackTags::TAG_STATUS_COMBO_PASSIVE) && IsValid(PassiveAttackMontageSet))
	{
		MontageToPlay = PassiveAttackMontageSet->Montage;
		HitEventTime = PassiveAttackMontageSet->Timing;
		AdjustMontageRateAndEventTime();
		return;
	}

	Super::SetMontageToPlay();
}

void UGA_Akali_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// The next attack starts over from the usual range and montage.
	ShouldPlayAttackAfterKick = false;
	AbilityRange = CachedAbilityRange;
}
