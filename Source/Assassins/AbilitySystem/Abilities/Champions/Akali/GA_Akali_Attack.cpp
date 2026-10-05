#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Attack.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "Character/AssassinsCharacter.h"
#include "NativeGameplayTags.h"

namespace AkaliAttackTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_PASSIVE, "Status.Combo.Passive");

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ABILITY3, "Status.Ability.Ability3");
};

void UGA_Akali_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();

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
		if (IsValid(AbilityTargetActor))
		{
			ApplyGameplayEffectSpecToTargetActor(MakeEffectSpecHandle(PassiveDamageEffectClass), AbilityTargetActor);
		}

		FGameplayTagContainer PassiveTags;
		PassiveTags.AddTag(AkaliAttackTags::TAG_STATUS_COMBO_PASSIVE);
		BP_RemoveGameplayEffectFromOwnerWithGrantedTags(PassiveTags, -1);
	}

	Super::OnAttackHit();
}

void UGA_Akali_Attack::SetMontageToPlay()
{
	// 킥 뒤 공격은 패시브가 남아 있을 때도 요청되므로 먼저 본다.
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

	ShouldPlayAttackAfterKick = false;
	AbilityRange = CachedAbilityRange;
}
