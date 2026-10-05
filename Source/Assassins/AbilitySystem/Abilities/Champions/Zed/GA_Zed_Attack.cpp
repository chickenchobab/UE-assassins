#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_Attack.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "Character/AssassinsCharacter.h"
#include "NativeGameplayTags.h"

namespace ZedAttackTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_ZED_PASSIVE, "Status.Target.Zed.Passive");
};

namespace
{
	static constexpr double PassiveHealthRatio = 0.5;
};

void UGA_Zed_Attack::PrePlayMontage()
{
	IsPassiveApplied = CanApplyPassive(AbilityTargetActor);

	Super::PrePlayMontage();
}

void UGA_Zed_Attack::OnAttackHit()
{
	Super::OnAttackHit();

	if (IsPassiveApplied && IsValid(AbilityTargetActor))
	{
		ApplyGameplayEffectSpecToTargetActor(MakeEffectSpecHandle(PassiveDamageEffectClass), AbilityTargetActor);
	}
}

void UGA_Zed_Attack::SetMontageToPlay()
{
	if (IsPassiveApplied && IsValid(PassiveAttackMontageData))
	{
		MontageToPlay = PassiveAttackMontageData->Montage;
		HitEventTime = PassiveAttackMontageData->Timing;
		return;
	}

	Super::SetMontageToPlay();
}

bool UGA_Zed_Attack::CanApplyPassive(const AActor* TargetActor) const
{
	const AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(TargetActor);
	if (TargetCharacter == nullptr)
	{
		return false;
	}

	const UAssassinsAbilitySystemComponent* TargetASC = TargetCharacter->GetAssassinsAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		return false;
	}

	if (TargetASC->HasMatchingGameplayTag(ZedAttackTags::TAG_STATUS_TARGET_ZED_PASSIVE))
	{
		return false;
	}

	bool bFoundHealth = false;
	const float TargetHealth = TargetASC->GetGameplayAttributeValue(UAssassinsHealthSet::GetHealthAttribute(), bFoundHealth);

	bool bFoundMaxHealth = false;
	const float TargetMaxHealth = TargetASC->GetGameplayAttributeValue(UAssassinsHealthSet::GetMaxHealthAttribute(), bFoundMaxHealth);

	if (TargetMaxHealth <= 0.0f)
	{
		return false;
	}

	return TargetHealth <= (TargetMaxHealth * PassiveHealthRatio);
}
