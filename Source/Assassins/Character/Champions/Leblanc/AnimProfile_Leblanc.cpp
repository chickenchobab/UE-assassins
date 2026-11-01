#include "Character/Champions/Leblanc/AnimProfile_Leblanc.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AssassinsChampionAnimInstance.h"

void UAnimProfile_Leblanc::ChooseStartAnim(UAssassinsChampionAnimInstance& AnimInstance)
{
	if (AnimInstance.HasStartSequence && !AnimInstance.IsMontageBlendingOut)
	{
		return;
	}

	const bool bDashBlendingOut = AnimInstance.IsMontageBlendingOut && (AnimInstance.BlendingOutMontage == DashMontage);
	AnimInstance.HasStartSequence = bDashBlendingOut;
	if (bDashBlendingOut)
	{
		StartSequence = StartSequenceAfterDash;
	}
}

void UAnimProfile_Leblanc::ChooseStopAnim(UAssassinsChampionAnimInstance& AnimInstance)
{
	if (AnimInstance.HasStopSequence && !AnimInstance.IsMontageBlendingOut)
	{
		return;
	}

	const bool bDashBlendingOut = AnimInstance.IsMontageBlendingOut && (AnimInstance.BlendingOutMontage == DashMontage);
	AnimInstance.HasStopSequence = true;
	StopSequence = bDashBlendingOut ? StopSequenceAfterDash : DefaultStopSequence;
}
