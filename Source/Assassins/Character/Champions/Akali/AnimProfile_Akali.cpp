#include "Character/Champions/Akali/AnimProfile_Akali.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AssassinsChampionAnimInstance.h"
#include "NativeGameplayTags.h"

namespace AkaliAnimation
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_PASSIVE, "Status.Combo.Passive");

	using FSequenceMap = TMap<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimSequenceBase>>;

	const TObjectPtr<UAnimSequenceBase>* FindFor(UAnimMontage* Montage, const FSequenceMap* First, const FSequenceMap& Second)
	{
		const TObjectPtr<UAnimSequenceBase>* Found = First ? First->Find(Montage) : nullptr;
		return Found ? Found : Second.Find(Montage);
	}
}

void UAnimProfile_Akali::UpdateTags(const UAbilitySystemComponent* AbilitySystem)
{
	bHasPassive = AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AkaliAnimation::TAG_STATUS_COMBO_PASSIVE);
}

void UAnimProfile_Akali::ChooseStartAnim(UAssassinsChampionAnimInstance& AnimInstance)
{
	using namespace AkaliAnimation;

	if (AnimInstance.HasStartSequence && !AnimInstance.IsMontageBlendingOut)
	{
		return;
	}

	AnimInstance.HasStartSequence = true;
	if (AnimInstance.IsMontageBlendingOut)
	{
		if (const TObjectPtr<UAnimSequenceBase>* Start = FindFor(AnimInstance.BlendingOutMontage, bHasPassive ? &StartSequenceMapWithPassive : nullptr, StartSequenceMap))
		{
			StartSequence = *Start;
			return;
		}
	}
	AnimInstance.HasStartSequence = false;
}

void UAnimProfile_Akali::ChooseStopAnim(UAssassinsChampionAnimInstance& AnimInstance)
{
	using namespace AkaliAnimation;

	// 패시브를 얻는 순간 미리 고른 정지를 버린다. 몽타주의 정지가 재생되기 때문이다.
	const bool bGotPassive = bHasPassive && !bHadPassiveLastUpdate;
	bHadPassiveLastUpdate = bHasPassive;
	if (bGotPassive)
	{
		AnimInstance.HasStopSequence = false;
		return;
	}

	// 한 번 고르고, 몽타주가 블렌드아웃할 때마다 다시 고른다. 한 정지 도중에는 바꾸지 않는다.
	if (AnimInstance.HasStopSequence && !AnimInstance.IsMontageBlendingOut)
	{
		return;
	}

	AnimInstance.HasStopSequence = true;
	bTakeStopInTurn = false;
	if (AnimInstance.IsMontageBlendingOut)
	{
		if (const TObjectPtr<UAnimSequenceBase>* Stop = FindFor(AnimInstance.BlendingOutMontage, bHasPassive ? &StopSequenceMapWithPassive : nullptr, StopSequenceMap))
		{
			StopSequence = *Stop;
			return;
		}
	}

	// 패시브 중에는 정지 없이 멈추고, 패시브를 주는 몽타주 뒤에도 그렇다.
	const bool bPassiveMontageBlendingOut = AnimInstance.IsMontageBlendingOut && (AnimInstance.BlendingOutMontage == PassiveMontage);
	if ((StopSequences.Num() > 0) && !bHasPassive && !bPassiveMontageBlendingOut)
	{
		bTakeStopInTurn = true;
	}
	else
	{
		AnimInstance.HasStopSequence = false;
	}
}

void UAnimProfile_Akali::HandleStopAnimStarted()
{
	if (bTakeStopInTurn)
	{
		StopSequence = TakeInTurn(StopSequences, CurrentStopSequenceIndex);
	}
}

bool UAnimProfile_Akali::ShouldOverrideIdleSequence(const UAssassinsChampionAnimInstance& AnimInstance) const
{
	return bHasPassive;
}

bool UAnimProfile_Akali::ShouldOverrideCycleSequence(const UAssassinsChampionAnimInstance& AnimInstance) const
{
	return bHasPassive;
}

UAnimSequenceBase* UAnimProfile_Akali::GetIdleSequenceOverride() const
{
	return IdleSequenceOverride;
}

UAnimSequenceBase* UAnimProfile_Akali::GetWalkSequenceOverride() const
{
	return WalkSequenceOverride;
}

UAnimSequenceBase* UAnimProfile_Akali::GetRunSequenceOverride() const
{
	return RunSequenceOverride;
}
