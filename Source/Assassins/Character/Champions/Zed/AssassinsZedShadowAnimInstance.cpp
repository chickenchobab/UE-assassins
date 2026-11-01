#include "Character/Champions/Zed/AssassinsZedShadowAnimInstance.h"
#include "AnimationStateMachineLibrary.h"

namespace ZedShadowAnimation
{
	const FName MontageToIdleState(TEXT("MontageToIdle"));
}

void UAssassinsZedShadowAnimInstance::RemoveMontageCompletionFlag(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	FAnimationStateMachineReference StateMachine;
	EAnimNodeReferenceConversionResult Result;
	UAnimationStateMachineLibrary::ConvertToAnimationStateMachine(Node, StateMachine, Result);
	if ((Result == EAnimNodeReferenceConversionResult::Succeeded) && (UAnimationStateMachineLibrary::GetState(Context, StateMachine) == ZedShadowAnimation::MontageToIdleState))
	{
		IsMontageEnded = false;
	}
}
