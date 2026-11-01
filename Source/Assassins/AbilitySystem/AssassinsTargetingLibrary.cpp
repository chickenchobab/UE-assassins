#include "AbilitySystem/AssassinsTargetingLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/AssassinsCharacter.h"
#include "NativeGameplayTags.h"

namespace AssassinsTargeting
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_DEATH, "Status.Death");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_UNTARGETABLE, "Status.Untargetable");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_MINION, "Status.Target.Minion");
}

double UAssassinsTargetingLibrary::GetDistance2DToTargetEdge(const AActor* Target, const FVector& FromLocation)
{
	if (Target == nullptr)
	{
		return 0.0;
	}

	// 구조물은 중심이 폰을 막는 충돌 안이라 가장자리까지 잰다. 대상 추적·사거리 판정과 같은 기준이다
	// (UAssassinsTargetChasingComponent::ChaseTarget, UAbility_UnitTargeted::IsTargetInRange).
	double Distance = FVector::Dist2D(FromLocation, Target->GetActorLocation());
	if (!Target->IsA<AAssassinsCharacter>())
	{
		Distance -= Target->GetSimpleCollisionRadius();
	}

	return FMath::Max(Distance, 0.0);
}

bool UAssassinsTargetingLibrary::IsAttackableTarget(const AActor* Target)
{
	const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (AbilitySystem == nullptr)
	{
		return false;
	}

	FGameplayTagContainer Unattackable;
	Unattackable.AddTag(AssassinsTargeting::TAG_STATUS_DEATH);
	Unattackable.AddTag(AssassinsTargeting::TAG_STATUS_UNTARGETABLE);
	return !AbilitySystem->HasAnyMatchingGameplayTags(Unattackable);
}

void UAssassinsTargetingLibrary::MarkTargetedByMinion(AActor* Target)
{
	// 떼는 쪽(BTT_CancelAttackAndClearTarget)과 같이 캐릭터에만 붙인다.
	if (AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(Target))
	{
		TargetCharacter->AddGameplayTag(AssassinsTargeting::TAG_STATUS_TARGET_MINION);
	}
}
