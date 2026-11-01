#include "Animation/AssassinsAnimProfile.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "SequencePlayerLibrary.h"

namespace ChampionAnimProfile
{
	// 시퀀스 플레이어의 Sequence가 동적이어야 한다(그래프에서 Always Dynamic).
	void SetSequence(const FAnimNodeReference& Node, UAnimSequenceBase* Sequence)
	{
		EAnimNodeReferenceConversionResult Result;
		const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
		USequencePlayerLibrary::SetSequence(SequencePlayer, Sequence);
	}
}

void UAssassinsAnimProfile::SetupIdleAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, IdleSequence);
}

void UAssassinsAnimProfile::SetupIdleOverrideAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, GetIdleSequenceOverride());
}

void UAssassinsAnimProfile::SetupWalkAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, WalkSequence);
}

void UAssassinsAnimProfile::SetupWalkOverrideAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, GetWalkSequenceOverride());
}

void UAssassinsAnimProfile::SetupRunAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, RunSequence);
}

void UAssassinsAnimProfile::SetupRunOverrideAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, GetRunSequenceOverride());
}

void UAssassinsAnimProfile::SetupStartAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, StartSequence);
}

void UAssassinsAnimProfile::SetupStopAnim(const FAnimNodeReference& Node) const
{
	ChampionAnimProfile::SetSequence(Node, StopSequence);
}

void UAssassinsAnimProfile::SetupIdleBreakAnim(const FAnimNodeReference& Node)
{
	ChampionAnimProfile::SetSequence(Node, IdleBreaks.IsValidIndex(CurrentIdleBreakIndex) ? IdleBreaks[CurrentIdleBreakIndex].Get() : nullptr);

	++CurrentIdleBreakIndex;
	if (CurrentIdleBreakIndex >= IdleBreaks.Num())
	{
		CurrentIdleBreakIndex = 0;
	}
}

UAnimSequenceBase* UAssassinsAnimProfile::TakeInTurn(const TArray<TObjectPtr<UAnimSequenceBase>>& Sequences, int32& Index)
{
	UAnimSequenceBase* Sequence = Sequences.IsValidIndex(Index) ? Sequences[Index].Get() : nullptr;

	++Index;
	if (Index >= Sequences.Num())
	{
		Index = 0;
	}
	return Sequence;
}
