#include "AbilitySystem/Abilities/Champions/Akali/Ability_Akali_R1.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/AssassinsInterframeHitCheckSubsystem.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"
#include "GameFramework/Character.h"
#include "NativeGameplayTags.h"
#include "Player/AssassinsPlayerController.h"

namespace AkaliUltimate
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ULTIMATE_RECAST, "Event.Champion.Akali.UltimateRecast");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY4_1, "Status.Combo.R.1");

	// 서버가 소유 클라이언트에게 돌진의 대상 명중을 알린다. GameCustom1은 UAbility_UnitTargeted의 도착이 쓴다.
	static constexpr EAbilityCustomReplicatedEvent HitNotifyEvent = EAbilityCustomReplicatedEvent::GameCustom2;

	static constexpr EAbilityCustomReplicatedEvent HitPredictionEvent = EAbilityCustomReplicatedEvent::GameCustom3;

	// 대상이 돌진 경로에서 판정 모양 범위를 벗어나도 서버가 클라이언트가 본 명중을 받아들이는 거리. 명중 소식이 오가는 동안 대상이 걷는 정도다.
	static constexpr double HitPredictionTolerance = 150.0;

	static constexpr double DirectionTolerance = 1.e-4;
};

UAbility_Akali_R1::UAbility_Akali_R1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbility_Akali_R1::PrePlayMontage()
{
	Super::PrePlayMontage();

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsActive() || (AvatarActor == nullptr))
	{
		return;
	}

	// 끝날 때 남은 돌진 태스크가 OnCancelled를 다시 부르므로 EndAbility가 아니라 여기서 초기화한다.
	HasDashBeenCut = false;

	DashStartLocation = AvatarActor->GetActorLocation();
	const FVector StartToTarget = AbilityTargetActor->GetActorLocation() - DashStartLocation;
	ToTarget = FVector(StartToTarget.X, StartToTarget.Y, 0.0);

	const FVector DashLocation = DashStartLocation + (ToTarget.GetSafeNormal2D(AkaliUltimate::DirectionTolerance) * (DashDistance + ToTarget.Size2D()));

	UAbilityTask_DashTo* DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, DashLocation, DashSpeed, 0.0f,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
	DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashCancelled);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->ReadyForActivation();
}

FGameplayTagContainer UAbility_Akali_R1::GetPendingCastTags() const
{
	// 시전 시간 동안만 붙드는 태그. 롤처럼 E·W는 시전 시간이 끝나 돌진이 시작되면 명중 전에도 쓸 수 있다.
	// Status.Dashing 대신 Q와 이동 클릭도 막는다. Status.Dashing은 돌진이 더하지 않고 설정해서 미리 붙들 수 없고,
	// 없으면 그사이 누른 Q가 시작돼 돌진에 끊긴다.
	FGameplayTagContainer PendingCastTags(AkaliUltimate::TAG_STATUS_COMBO_ABILITY4_1);
	PendingCastTags.AddTag(AssassinsGameplayTags::Status_Channeling);
	return PendingCastTags;
}

void UAbility_Akali_R1::PostPlayMontage()
{
	// 바로 맞는 대상이라도 명중 몽타주가 돌진 몽타주 위에 재생되도록, 돌진 몽타주가 시작된 뒤에 판정을 시작한다.
	// 모양 안에 이미 선 적(대상 포함)은 첫 이동이 시작하면서 찾는다.
	BeginHits();

	if (K2_HasAuthority())
	{
		if (!IsLocallyControlled())
		{
			UAbilityTask_WaitReplicatedEvent* HitPredictionTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliUltimate::HitPredictionEvent);
			HitPredictionTask->OnEvent.AddDynamic(this, &ThisClass::OnClientPredictedHit);
			HitPredictionTask->ReadyForActivation();
		}
		return;
	}

	UAbilityTask_WaitReplicatedEvent* HitNotifyTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliUltimate::HitNotifyEvent);
	HitNotifyTask->OnEvent.AddDynamic(this, &ThisClass::ClientGetNotifiedOfHitEvent);
	HitNotifyTask->ReadyForActivation();
}

void UAbility_Akali_R1::BeginHits()
{
	ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (AvatarCharacter == nullptr)
	{
		return;
	}

	if (K2_HasAuthority())
	{
		UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this);
		if (HitCheck == nullptr)
		{
			return;
		}

		const TWeakObjectPtr<ThisClass> WeakThis(this);
		HitShapeHandle = HitCheck->BeginDashHitShape(AvatarCharacter, UAssassinsChampionSkillState_Akali::GetUltimateHitShape(),
			[WeakThis](AActor* Target)
			{
				const ThisClass* Self = WeakThis.Get();
				return (Self != nullptr) && Self->IsValidEnemy(Target) && !Self->HitActors.Contains(Target);
			},
			[WeakThis](AActor* Target)
			{
				if (ThisClass* Self = WeakThis.Get())
				{
					Self->HandleHit(Target);
				}
			});
		return;
	}

	if (IsValid(AbilityTargetActor))
	{
		TargetLocationBefore = AbilityTargetActor->GetActorLocation();
		AvatarCharacter->OnCharacterMovementUpdated.AddDynamic(this, &ThisClass::OnAvatarMoved);
	}
}

void UAbility_Akali_R1::EndHits()
{
	if (HitShapeHandle != 0)
	{
		if (UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this))
		{
			HitCheck->EndDashHitShape(HitShapeHandle);
		}
		HitShapeHandle = 0;
	}

	if (ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		AvatarCharacter->OnCharacterMovementUpdated.RemoveDynamic(this, &ThisClass::OnAvatarMoved);
	}
}

void UAbility_Akali_R1::OnAvatarMoved(float DeltaSeconds, FVector OldLocation, FVector OldVelocity)
{
	const ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (HasHitAbilityTarget || (AvatarCharacter == nullptr) || !IsValid(AbilityTargetActor))
	{
		return;
	}

	// 서버 보정으로 다시 돌린 이동은 처음 했을 때 이미 지켜봤다.
	if (AvatarCharacter->bClientUpdating)
	{
		return;
	}

	// 이 클라이언트가 보는 같은 프레임의 아칼리 이동과 대상 이동을 비교한다. 둘은 양 끝에서는 떨어져 있어도 두 프레임 사이에 스칠 수 있다.
	const FAssassinsInterframeHitShape Shape = UAssassinsChampionSkillState_Akali::GetUltimateHitShape();
	FAssassinsSweptPair Pair;
	Pair.StartA = OldLocation;
	Pair.EndA = AvatarCharacter->GetActorLocation();
	Pair.RadiusA = Shape.Radius;
	Pair.HalfHeightA = Shape.HalfHeight;
	Pair.StartB = TargetLocationBefore;
	Pair.EndB = AbilityTargetActor->GetActorLocation();
	AbilityTargetActor->GetSimpleCollisionCylinder(Pair.RadiusB, Pair.HalfHeightB);
	TargetLocationBefore = Pair.EndB;

	if (IsActive() && IsValidEnemy(AbilityTargetActor) && AssassinsInterframeHitCheck::SolvePair(Pair).bHit)
	{
		PredictTargetHit();
	}
}

void UAbility_Akali_R1::HandleHit(AActor* TargetActor)
{
	// 명중 판정은 서버가 한다. 소유 클라이언트는 대상 명중만 미리 본다(OnAvatarMoved).
	if (!IsActive() || !K2_HasAuthority() || !IsValidEnemy(TargetActor) || HitActors.Contains(TargetActor))
	{
		return;
	}

	HitActors.Add(TargetActor);
	ApplyGameplayEffectSpecToTargetActor(MakeEffectSpecHandle(DamageEffectClass), TargetActor);

	HasHitAbilityTarget = HasHitAbilityTarget || (TargetActor == AbilityTargetActor);

	// 돌진이 끊긴 뒤에 판정된 명중은 끊기기 전에 닿은 것이라 피해만 준다. 이어 갈 돌진이 없고, 명중 몽타주가 끊은 쪽(E)의 몽타주를 덮는다.
	if (!HasHitAbilityTarget || HasServerNotifiedClientHitEvent || HasDashBeenCut)
	{
		return;
	}

	HasServerNotifiedClientHitEvent = true;
	if (!IsLocallyControlled())
	{
		ClientSetReplicatedEvent(AkaliUltimate::HitNotifyEvent);
	}

	PlayHitDash();
}

void UAbility_Akali_R1::PredictTargetHit()
{
	if (HasHitAbilityTarget)
	{
		return;
	}

	HasHitAbilityTarget = true;

	// 서버 왕복을 기다리지 않고 이 클라이언트가 본 명중으로 명중 돌진을 시작한다. 서버는 소식을 받는 즉시 명중을 처리한다.
	SendPredictedEventToServer(AkaliUltimate::HitPredictionEvent, [this]() { PlayHitDash(); });
}

void UAbility_Akali_R1::ClientGetNotifiedOfHitEvent()
{
	if (HasHitAbilityTarget)
	{
		return;
	}

	HasHitAbilityTarget = true;
	if (!HasDashBeenCut)
	{
		PlayHitDash();
	}
}

void UAbility_Akali_R1::OnClientPredictedHit()
{
	if (HasHitAbilityTarget || !IsTargetAlongDash())
	{
		return;
	}

	HandleHit(AbilityTargetActor);
}

bool UAbility_Akali_R1::IsTargetAlongDash() const
{
	if (!IsValid(AbilityTargetActor))
	{
		return false;
	}

	const FVector DashDirection = ToTarget.GetSafeNormal2D(AkaliUltimate::DirectionTolerance);
	const FVector DashStart(DashStartLocation.X, DashStartLocation.Y, 0.0);
	const FVector DashEnd = DashStart + (DashDirection * (DashDistance + ToTarget.Size2D()));

	const FVector TargetLocation = AbilityTargetActor->GetActorLocation();
	const double TargetDistance = FMath::PointDistToSegment(FVector(TargetLocation.X, TargetLocation.Y, 0.0), DashStart, DashEnd);
	const double Reach = UAssassinsChampionSkillState_Akali::GetUltimateHitShape().Radius + AbilityTargetActor->GetSimpleCollisionRadius();
	return TargetDistance <= (Reach + AkaliUltimate::HitPredictionTolerance);
}

void UAbility_Akali_R1::PlayHitDash()
{
	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnHitDashSynced);
	SyncTask->ReadyForActivation();
}

void UAbility_Akali_R1::OnHitDashSynced()
{
	// 서버가 소유 클라이언트의 신호를 기다리는 사이에 돌진이 끊길 수 있다.
	if (HasDashBeenCut)
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* HitMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, HitDashMontage, 1.0f, NAME_None, true);
	HitMontageTask->ReadyForActivation();
}

void UAbility_Akali_R1::OnDashCancelled()
{
	HasDashBeenCut = true;
	OnDashEnded();
}

void UAbility_Akali_R1::OnDashEnded()
{
	// 모양은 돌진 중에만 맞힌다. 그 이동들에 대해 아직 판정 중인 명중은 뒤에 들어온다.
	EndHits();

	if (IsLocallyControlled())
	{
		WaitAndHandleRecast();
	}
}

void UAbility_Akali_R1::WaitAndHandleRecast()
{
	UAbilityTask_WaitDelay* PermissionTask = UAbilityTask_WaitDelay::WaitDelay(this, RecastPermissionTime);
	PermissionTask->OnFinish.AddDynamic(this, &ThisClass::OnRecastPermissionEnded);
	PermissionTask->ReadyForActivation();

	UAbilityTask_WaitDelay* RecastDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, RecastDelay);
	RecastDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnRecastAllowed);
	RecastDelayTask->ReadyForActivation();
}

void UAbility_Akali_R1::OnRecastPermissionEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAbility_Akali_R1::OnRecastAllowed()
{
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UAbility_Akali_R1::OnRecastInputReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastInputPressed);
	PressTask->ReadyForActivation();
}

void UAbility_Akali_R1::OnRecastInputPressed(float TimeWaited)
{
	// 다른 어빌리티 시전 중(Status.Channeling)에 누르면 재시전이 시전을 끊으므로(Q 취소) 거절하고 다음 입력을 기다린다.
	// 소유 클라이언트만 판단하고, 서버는 보내온 재시전만 본다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(AssassinsGameplayTags::Status_Channeling))
	{
		OnRecastAllowed();
		return;
	}

	Recast();
}

void UAbility_Akali_R1::Recast()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC == nullptr)
	{
		return;
	}

	// 재시전은 이 이벤트로 시작하는 별도 어빌리티다. 갈 곳을 컨텍스트에서 읽고, 이벤트 데이터가 활성화와 함께 가서 서버에서도 읽는다.
	RecastEventContext = ASC->MakeEffectContext();
	if (AAssassinsPlayerController* AssassinsPlayerController = GetAssassinsPlayerControllerFromActorInfo())
	{
		FHitResult CursorHitResult;
		AssassinsPlayerController->GetHitResultUnderCursorByChannel(TraceTypeQuery1, true, CursorHitResult);
		RecastEventContext.AddHitResult(CursorHitResult, false);
	}

	FGameplayEventData Payload;
	Payload.ContextHandle = RecastEventContext;
	SendGameplayEvent(AkaliUltimate::TAG_EVENT_ULTIMATE_RECAST, Payload);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAbility_Akali_R1::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	EndHits();

	HitActors.Empty();
	HasHitAbilityTarget = false;
	HasServerNotifiedClientHitEvent = false;
	RecastEventContext = FGameplayEffectContextHandle();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
