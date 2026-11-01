#include "Character/Champions/Zed/AnimProfile_Zed.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AssassinsChampionAnimInstance.h"

void UAnimProfile_Zed::ChooseStartAnim(UAssassinsChampionAnimInstance& AnimInstance)
{
	if (!AnimInstance.HasStartSequence)
	{
		StartSequence = DefaultStartSequence;
		AnimInstance.HasStartSequence = true;
	}
}

void UAnimProfile_Zed::ChooseStopAnim(UAssassinsChampionAnimInstance& AnimInstance)
{
	if (AnimInstance.HasStopSequence)
	{
		return;
	}

	AnimInstance.HasStopSequence = (StopSequences.Num() > 0);
}

void UAnimProfile_Zed::HandleStopAnimStarted()
{
	StopSequence = TakeInTurn(StopSequences, CurrentStopSequenceIndex);
}
