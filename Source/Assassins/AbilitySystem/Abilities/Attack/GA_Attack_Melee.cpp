#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"

void UGA_Attack_Melee::OnAttackHit()
{
	// 스윙 중 대상이 죽었을 수 있고, 어빌리티 시스템이 없는 액터에 적용하면 check에 걸린다.
	AttackDamageSpecHandle = MakeEffectSpecHandle(GetAttackEffectClass());
	if (IsValid(AbilityTargetActor))
	{
		ApplyGameplayEffectSpecToTargetActor(AttackDamageSpecHandle, AbilityTargetActor);
	}

	SendActivateAttackEvent(AbilityTargetActor);

	Super::OnAttackHit();

	if (IsMontageEnded)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
