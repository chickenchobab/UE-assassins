// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Ability4_Recast.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "Animation/AssassinsAnimInstance.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CurveTable.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "NativeGameplayTags.h"

namespace AkaliUltimateRecast
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ANIMNOTIFY_PAUSE_MONTAGE, "Event.AnimNotify.PauseMontage");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_ULTIMATE_RECAST_DASH, "GameplayCue.Champion.Akali.UltimateRecastDash");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_ULTIMATE_RECAST_HIT, "GameplayCue.Champion.Akali.UltimateRecastHit");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SETBYCALLER_DAMAGE_MULTIPLICATION, "SetByCaller.Akali.DamageMultiplication");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY4_2, "Status.Combo.Ability4.2");
};

UGA_Akali_Ability4_Recast::UGA_Akali_Ability4_Recast(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DamagePerMissingHealthRow = TEXT("TotalDamagePercent");
}

void UGA_Akali_Ability4_Recast::PrePlayMontage()
{
	// Faces the cursor.
	Super::PrePlayMontage();

	if (K2_HasAuthority())
	{
		// Enemies overlapping already count.
		AcquireCapsule();
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsActive() || (AvatarActor == nullptr))
	{
		return;
	}

	AddTagToAvatar(AkaliUltimateRecast::TAG_STATUS_COMBO_ABILITY4_2);

	// The full range, the way the cursor points.
	const FVector DashLocation = AvatarActor->GetActorLocation() + (CursorLookAtRotation.Vector() * AbilityRange);

	UAbilityTask_DashTo* DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, DashLocation, DashSpeed, /*InAcceptRadius*/ 0.0f,
		ERootMotionFinishVelocityMode::ClampVelocity, /*SetVelocityOnFinish*/ FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
	DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->ReadyForActivation();

	if (!IsActive())
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Normal = CursorLookAtRotation.Vector();
	K2_ExecuteGameplayCueWithParams(AkaliUltimateRecast::TAG_GAMEPLAYCUE_ULTIMATE_RECAST_DASH, CueParameters);
}

void UGA_Akali_Ability4_Recast::PostPlayMontage()
{
	UAbilityTask_WaitGameplayEvent* PauseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, AkaliUltimateRecast::TAG_EVENT_ANIMNOTIFY_PAUSE_MONTAGE, /*OptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
	PauseTask->EventReceived.AddDynamic(this, &ThisClass::OnPauseMontageNotify);
	PauseTask->ReadyForActivation();
}

void UGA_Akali_Ability4_Recast::OnPauseMontageNotify(FGameplayEventData Payload)
{
	// The montage holds its pose until the dash is over.
	if (UAssassinsAnimInstance* AnimInstance = GetAssassinsAnimInstanceFromActorInfo())
	{
		AnimInstance->Montage_Pause(MontageToPlay);
	}
}

void UGA_Akali_Ability4_Recast::OnDashEnded()
{
	RemoveTagFromAvatar(AkaliUltimateRecast::TAG_STATUS_COMBO_ABILITY4_2);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability4_Recast::AcquireCapsule()
{
	UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(GetAvatarActorFromActorInfo());
	if (AkaliState == nullptr)
	{
		return;
	}

	AbilityCapsule = AkaliState->AcquireUltimateCapsule(this);
	if (AbilityCapsule == nullptr)
	{
		return;
	}

	AbilityCapsule->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnTargetHit);
	if (ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		AvatarCharacter->OnCharacterMovementUpdated.AddDynamic(this, &ThisClass::OnAvatarMoved);
	}

	TArray<AActor*> OverlappingActors;
	AbilityCapsule->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		HandleOverlappedActor(OverlappingActor);
	}
}

void UGA_Akali_Ability4_Recast::ReleaseCapsule()
{
	if (AbilityCapsule == nullptr)
	{
		return;
	}

	AbilityCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnTargetHit);
	if (ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		AvatarCharacter->OnCharacterMovementUpdated.RemoveDynamic(this, &ThisClass::OnAvatarMoved);
	}
	AbilityCapsule = nullptr;

	if (UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(GetAvatarActorFromActorInfo()))
	{
		AkaliState->ReleaseUltimateCapsule(this);
	}
}

void UGA_Akali_Ability4_Recast::OnTargetHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandleOverlappedActor(OtherActor);
}

void UGA_Akali_Ability4_Recast::OnAvatarMoved(float DeltaSeconds, FVector OldLocation, FVector OldVelocity)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if ((AbilityCapsule == nullptr) || (AvatarActor == nullptr) || (World == nullptr))
	{
		return;
	}

	// The capsule keeps its place on Akali: where it was as the move started.
	const FVector SweepEnd = AbilityCapsule->GetComponentLocation();
	const FVector SweepStart = OldLocation + (SweepEnd - AvatarActor->GetActorLocation());
	if (SweepStart.Equals(SweepEnd))
	{
		return;
	}

	// Pawns, which the capsule overlaps(AcquireUltimateCapsule).
	TArray<FHitResult> Hits;
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AkaliUltimateRecastSweep), /*bTraceComplex*/ false, AvatarActor);
	World->SweepMultiByObjectType(Hits, SweepStart, SweepEnd, AbilityCapsule->GetComponentQuat(), FCollisionObjectQueryParams(ECC_Pawn), AbilityCapsule->GetCollisionShape(), QueryParams);
	for (const FHitResult& Hit : Hits)
	{
		HandleOverlappedActor(Hit.GetActor());
	}
}

void UGA_Akali_Ability4_Recast::HandleOverlappedActor(AActor* OverlappedActor)
{
	if (!IsActive() || !IsValidEnemy(OverlappedActor) || OverlappedActors.Contains(OverlappedActor))
	{
		return;
	}

	OverlappedActors.Add(OverlappedActor);

	DamageSpecHandle = MakeEffectSpecHandle(DamageEffectClass);
	if (FGameplayEffectSpec* DamageSpec = DamageSpecHandle.Data.Get())
	{
		DamageSpec->SetSetByCallerMagnitude(AkaliUltimateRecast::TAG_SETBYCALLER_DAMAGE_MULTIPLICATION, CalculateDamageIncrease(OverlappedActor));
	}
	ApplyGameplayEffectSpecToTargetActor(DamageSpecHandle, OverlappedActor);

	if (!HasPlayedHitEffect)
	{
		K2_ExecuteGameplayCue(AkaliUltimateRecast::TAG_GAMEPLAYCUE_ULTIMATE_RECAST_HIT, FGameplayEffectContextHandle());
		HasPlayedHitEffect = true;
	}
}

float UGA_Akali_Ability4_Recast::CalculateDamageIncrease(AActor* Actor) const
{
	const UAssassinsHealthComponent* HealthComponent = UAssassinsHealthComponent::FindHealthComponent(Actor);
	if ((HealthComponent == nullptr) || (DamagePerMissingHealthTable == nullptr))
	{
		return 0.0f;
	}

	const FRealCurve* Curve = DamagePerMissingHealthTable->FindCurve(DamagePerMissingHealthRow, TEXT("Akali recast damage per missing health"));
	return Curve ? Curve->Eval(1.0f - HealthComponent->GetHealthNormalized()) : 0.0f;
}

void UGA_Akali_Ability4_Recast::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ReleaseCapsule();

	OverlappedActors.Empty();
	HasPlayedHitEffect = false;
	DamageSpecHandle = FGameplayEffectSpecHandle();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
