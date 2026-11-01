#include "Animation/AssassinsChampionAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AnimationStateMachineLibrary.h"
#include "AnimExecutionContextLibrary.h"
#include "Animation/AnimMontage.h"
#include "Animation/AssassinsAnimProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "NativeGameplayTags.h"

namespace ChampionAnimation
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY, "Status.Ability");

	constexpr double StillTolerance = 0.000001;
}

UAssassinsChampionAnimInstance::UAssassinsChampionAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAssassinsChampionAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	Super::InitializeWithAbilitySystem(ASC);

	AbilitySystem = ASC;
}

void UAssassinsChampionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OnMontageBlendingOut.AddUniqueDynamic(this, &ThisClass::HandleMontageBlendingOut);
}

void UAssassinsChampionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AActor* OwningActor = GetOwningActor();
	OwnerLocation = OwningActor ? OwningActor->GetActorLocation() : FVector::ZeroVector;

	const APawn* PawnOwner = TryGetPawnOwner();
	OwnerVelocity = PawnOwner ? PawnOwner->GetVelocity() : FVector::ZeroVector;
	const UCharacterMovementComponent* Movement = PawnOwner ? Cast<UCharacterMovementComponent>(PawnOwner->GetMovementComponent()) : nullptr;
	OwnerAcceleration = Movement ? Movement->GetCurrentAcceleration() : FVector::ZeroVector;

	bAnyMontagePlayingThisFrame = IsAnyMontagePlaying();

	// 태그는 게임 스레드에서 프레임마다 한 번 읽는다. 워커 스레드는 그 프레임 동안 바뀌지 않는 값을 본다.
	const UAbilitySystemComponent* ASC = AbilitySystem.Get();
	GameplayTag_HasActivatedAbility = ASC && ASC->HasMatchingGameplayTag(ChampionAnimation::TAG_STATUS_ABILITY);
	if (AnimProfile)
	{
		AnimProfile->UpdateTags(ASC);
	}
}

void UAssassinsChampionAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	UpdateLocationData(DeltaSeconds);
	UpdateVelocityData();
	UpdateAccelerationData();
	UpdateMontageData();
	IsFirstUpdate = false;
}

void UAssassinsChampionAnimInstance::UpdateLocationData(double DeltaTime)
{
	DisplacementSinceLastUpdate = (OwnerLocation - WorldLocation).Size2D();
	WorldLocation = OwnerLocation;
	DisplacementSpeed = (DeltaTime != 0.0) ? (DisplacementSinceLastUpdate / DeltaTime) : 0.0;

	if (IsFirstUpdate)
	{
		DisplacementSinceLastUpdate = 0.0;
		DisplacementSpeed = 0.0;
	}
}

void UAssassinsChampionAnimInstance::UpdateVelocityData()
{
	WorldVelocity = OwnerVelocity * FVector(1.0, 1.0, 0.0);
	HasVelocity = !WorldVelocity.IsNearlyZero(ChampionAnimation::StillTolerance);
}

void UAssassinsChampionAnimInstance::UpdateAccelerationData()
{
	WorldAcceleration = OwnerAcceleration * FVector(1.0, 1.0, 0.0);
	HasAcceleration = !FMath::IsNearlyEqual(WorldAcceleration.SizeSquared2D(), 0.0, ChampionAnimation::StillTolerance);
}

void UAssassinsChampionAnimInstance::UpdateMontageData()
{
	IsMontagePlaying = bAnyMontagePlayingThisFrame;
}

void UAssassinsChampionAnimInstance::UpdateLocomotionSM(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->ChooseStartAnim(*this);
		AnimProfile->ChooseStopAnim(*this);
	}
}

void UAssassinsChampionAnimInstance::SetupIdleState(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	ChooseIdleBreakDelayTime();
	ResetIdleBreakTransition();
}

void UAssassinsChampionAnimInstance::UpdateIdleState(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	FAnimationStateResultReference State;
	EAnimNodeReferenceConversionResult Result;
	UAnimationStateMachineLibrary::ConvertToAnimationStateResult(Node, State, Result);
	if (!UAnimationStateMachineLibrary::IsStateBlendingOut(Context, State))
	{
		ProcessIdleBreakTransition(UAnimExecutionContextLibrary::GetDeltaTime(Context));
	}
}

void UAssassinsChampionAnimInstance::SetupIdleAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupIdleAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupIdleOverrideAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupIdleOverrideAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupWalkAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupWalkAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupWalkOverrideAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupWalkOverrideAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupRunAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupRunAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupRunOverrideAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupRunOverrideAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupStartAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupStartAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupStopAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::SetupIdleBreakAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (AnimProfile)
	{
		AnimProfile->SetupIdleBreakAnim(Node);
	}
}

void UAssassinsChampionAnimInstance::ResetStartAndStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	HasStartSequence = false;
	HasStopSequence = false;
}

void UAssassinsChampionAnimInstance::HandlePossibleMontageToStop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	IsMontageBlendingOut = false;

	// 정지 상태가 활성화될 때 불린다. 차례로 도는 정지는 고른 값이 한 바퀴에 여러 번 지워지므로, 여기서 골라야 재생될 때만 차례가 넘어간다.
	if (AnimProfile)
	{
		AnimProfile->HandleStopAnimStarted();
	}
}

void UAssassinsChampionAnimInstance::HandlePossibleMontageToCycle(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	HasStopSequence = false;
	IsMontageBlendingOut = false;
}

bool UAssassinsChampionAnimInstance::CanPlayIdleBreaks() const
{
	return AnimProfile && AnimProfile->HasIdleBreaks() && !(HasVelocity || GameplayTag_HasActivatedAbility || IsMontagePlaying);
}

bool UAssassinsChampionAnimInstance::ShouldOverrideIdleSequence() const
{
	return AnimProfile && AnimProfile->ShouldOverrideIdleSequence(*this);
}

bool UAssassinsChampionAnimInstance::ShouldOverrideCycleSequence() const
{
	return AnimProfile && AnimProfile->ShouldOverrideCycleSequence(*this);
}

void UAssassinsChampionAnimInstance::ChooseIdleBreakDelayTime()
{
	const int32 Seconds = (FMath::TruncToInt32(FMath::Abs(WorldLocation.X + WorldLocation.Y)) % 10) + 6;
	IdleBreakDelayTime = static_cast<double>(Seconds);
}

void UAssassinsChampionAnimInstance::ProcessIdleBreakTransition(double DeltaTime)
{
	if (CanPlayIdleBreaks())
	{
		TimeUntilNextIdleBreak -= DeltaTime;
	}
	else
	{
		ResetIdleBreakTransition();
	}
}

void UAssassinsChampionAnimInstance::ResetIdleBreakTransition()
{
	TimeUntilNextIdleBreak = IdleBreakDelayTime;
}

void UAssassinsChampionAnimInstance::HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	IsMontageBlendingOut = true;
	BlendingOutMontage = Montage;
}
