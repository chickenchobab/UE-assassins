// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Leblanc/GA_Leblanc_Ability2.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitRecastPress.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AssassinsGameplayTags.h"
#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"
#include "CollisionQueryParams.h"
#include "GameFramework/RootMotionSource.h"
#include "NativeGameplayTags.h"

namespace LeblancDistortion
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ABILITY4, "Ability.Ability4");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_MIMIC_ABILITY2, "Event.Champion.Leblanc.Mimic.Ability2");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ULTIMATE_RECAST, "Event.Champion.Leblanc.UltimateRecast");

	// The owning client of the mimic tells the server it returns.
	static constexpr EAbilityCustomReplicatedEvent ReturnEvent = EAbilityCustomReplicatedEvent::GameCustom1;

	// The hit is a column from the ground up to this height, where the dash ends.
	static constexpr double DamageTraceHeight = 300.0;
};

UGA_Leblanc_Ability2::UGA_Leblanc_Ability2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Leblanc_Ability2::PrePlayMontage()
{
	bDashFinished = false;
	bDashSynced = false;
	bRecastInputReleased = false;
	bReturnRequested = false;
	bReturnStarted = false;

	// Faces the cursor.
	Super::PrePlayMontage();

	UAssassinsChampionSkillState_Leblanc::SetMimicAbilityOf(*this, LeblancDistortion::TAG_EVENT_MIMIC_ABILITY2);

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	// Taken after facing the cursor: Leblanc comes back facing the way she dashed.
	ReturnLocation = AvatarActor->GetActorLocation();
	ReturnRotation = AvatarActor->GetActorRotation();

	FGameplayCueParameters CueParameters;
	CueParameters.Location = ReturnLocation;
	K2_AddGameplayCueWithParams(ReturnLocationCueTag, CueParameters, /*bRemoveOnAbilityEnd*/ true);

	DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, CursorLocationClamped, DashSpeed, /*InAcceptRadius*/ 0.0f,
		ERootMotionFinishVelocityMode::ClampVelocity, /*SetVelocityOnFinish*/ FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
	DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashCancelled);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::OnDashFinished);
	DashTask->ReadyForActivation();

	if (!IsActive())
	{
		return;
	}

	if (K2_HasAuthority() && !IsLocallyControlled())
	{
		WaitForRecast();
	}
	else if (IsLocallyControlled() && !IsMimic())
	{
		// The press that cast the ability has to be let go first. Watched for from the start, so that by the landing it is
		// known whether the input is down from it or from a press since(OnDashSynced).
		WaitForRecastInputRelease();
	}
}

bool UGA_Leblanc_Ability2::IsMimic() const
{
	return GetAssetTags().HasTagExact(LeblancDistortion::TAG_ABILITY_ABILITY4);
}

void UGA_Leblanc_Ability2::OnDashFinished()
{
	// The dash puts Leblanc on its target location as it finishes.
	HandleDashComplete(DashTask ? DashTask->GetCurrentTargetLocation() : CursorLocationClamped);
}

void UGA_Leblanc_Ability2::OnDashCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Leblanc_Ability2::HandleDashComplete(const FVector& EndLocation)
{
	if (bDashFinished)
	{
		return;
	}

	bDashFinished = true;
	DashEndLocation = EndLocation;

	// The one sync point of the activation: the owning client predicts the hit in its window, and the server hits
	// when the client did.
	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnDashSynced);
	SyncTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnDashSynced()
{
	bDashSynced = true;

	ApplyDashDamage();

	// The server heard the return during its dash: it follows the hit, as on the client.
	if (bReturnRequested)
	{
		StartReturn();
		return;
	}

	ReturnDurationTask = UAbilityTask_WaitDelay::WaitDelay(this, ReturnDuration);
	ReturnDurationTask->OnFinish.AddDynamic(this, &ThisClass::OnReturnDurationFinished);
	ReturnDurationTask->ReadyForActivation();

	if (!IsLocallyControlled())
	{
		return;
	}

	if (IsMimic())
	{
		WaitForRecast();
	}
	else if (bRecastInputReleased)
	{
		// The input is down from a press made since the cast was let go, and the press returns: one made the very frame
		// the dash ends is handled before the end, and would be lost to a wait for the input to be let go.
		WaitForReturnPress(/*bTestAlreadyPressed*/ true);
	}
	// Otherwise the press that cast the ability is still down: its release starts the wait for the return
	// (OnRecastInputReleased).
}

void UGA_Leblanc_Ability2::ApplyDashDamage()
{
	DamageSpecHandle = MakeEffectSpecHandle(DamageEffectClass);

	for (const FHitResult& Hit : SweepForEnemies(DashEndLocation, EffectRadius, LeblancDistortion::DamageTraceHeight, FCollisionObjectQueryParams(ECC_Pawn)))
	{
		AActor* HitActor = Hit.GetActor();
		if (ActorsToIgnore.Contains(HitActor))
		{
			continue;
		}

		ApplyGameplayEffectSpecToTargetActor(DamageSpecHandle, HitActor);
		ActorsToIgnore.Add(HitActor);
	}
}

void UGA_Leblanc_Ability2::WaitForRecast()
{
	if (IsMimic())
	{
		if (IsLocallyControlled())
		{
			// The ultimate sends its recast on the owning client only.
			UAbilityTask_WaitGameplayEvent* RecastTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, LeblancDistortion::TAG_EVENT_ULTIMATE_RECAST, /*OptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
			RecastTask->EventReceived.AddDynamic(this, &ThisClass::OnUltimateRecast);
			RecastTask->ReadyForActivation();
		}
		else
		{
			UAbilityTask_WaitReplicatedEvent* ReturnTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, LeblancDistortion::ReturnEvent);
			ReturnTask->OnEvent.AddDynamic(this, &ThisClass::OnReturnRequested);
			ReturnTask->ReadyForActivation();
		}
		return;
	}

	WaitForRecastInputRelease();
}

void UGA_Leblanc_Ability2::WaitForRecastInputRelease()
{
	// The input events replicate: the server hears the same release and press, and handles the press in its
	// prediction key.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnRecastInputReleased(float TimeHeld)
{
	bRecastInputReleased = true;

	// The owning client returns only once it landed, where the wait for the press starts(OnDashSynced).
	if (IsLocallyControlled() && !bDashSynced)
	{
		return;
	}

	WaitForReturnPress(/*bTestAlreadyPressed*/ false);
}

void UGA_Leblanc_Ability2::WaitForReturnPress(bool bTestAlreadyPressed)
{
	// A press during the cast of another ability does not return: the return would cut the cast short. The owning client
	// does not take it, so the server never hears of it either.
	UAbilityTask_WaitRecastPress* PressTask = UAbilityTask_WaitRecastPress::WaitRecastPress(this, FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling), bTestAlreadyPressed);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastInputPressed);
	PressTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnRecastInputPressed()
{
	DoReturn();
}

void UGA_Leblanc_Ability2::OnUltimateRecast(FGameplayEventData Payload)
{
	SendPredictedEventToServer(LeblancDistortion::ReturnEvent, [this]() { DoReturn(); });
}

void UGA_Leblanc_Ability2::OnReturnRequested()
{
	DoReturn();
}

void UGA_Leblanc_Ability2::DoReturn()
{
	if (bReturnRequested || !IsActive())
	{
		return;
	}

	bReturnRequested = true;

	if (bDashSynced)
	{
		StartReturn();
		return;
	}

	// Only the server gets here. The owning client can only return after its dash, but its last moves of the dash
	// may reach the server after the event. The dash is finished where it was headed, and the return follows its hit
	// (OnDashSynced), as on the client. Were the dash left going, it would carry Leblanc back to its target after
	// the moves of the client put her at the return location.
	if (!bDashFinished)
	{
		FinishDashEarly();
	}
}

void UGA_Leblanc_Ability2::FinishDashEarly()
{
	FVector EndLocation = CursorLocationClamped;
	if (IsValid(DashTask))
	{
		EndLocation = DashTask->GetCurrentTargetLocation();

		// Not a cancellation: ending the task only takes the root motion away.
		DashTask->OnCancelled.RemoveDynamic(this, &ThisClass::OnDashCancelled);
		DashTask->EndTask();
	}

	HandleDashComplete(EndLocation);
}

void UGA_Leblanc_Ability2::StartReturn()
{
	if (bReturnStarted)
	{
		return;
	}

	bReturnStarted = true;

	// The time to return is over: it must not cut the return short.
	if (IsValid(ReturnDurationTask))
	{
		ReturnDurationTask->EndTask();
	}
	ReturnDurationTask = nullptr;

	// Through the character movement: the server takes the teleport from the moves of the owning client.
	if (IsLocallyControlled())
	{
		SetAvatarLocationAndRotation(ReturnLocation, ReturnRotation);
	}

	K2_RemoveGameplayCue(ReturnLocationCueTag);

	UAbilityTask_PlayMontageAndWait* ReturnMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, ReturnMontage, /*Rate*/ 1.0f, NAME_None, /*bStopWhenAbilityEnds*/ true);
	ReturnMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnReturnMontageEnded);
	ReturnMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnReturnMontageEnded);
	ReturnMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnReturnMontageEnded);
	ReturnMontageTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnReturnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Leblanc_Ability2::OnReturnDurationFinished()
{
	K2_RemoveGameplayCue(ReturnLocationCueTag);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Leblanc_Ability2::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// The tasks end with the ability.
	DashTask = nullptr;
	ReturnDurationTask = nullptr;

	ActorsToIgnore.Empty();
	DamageSpecHandle = FGameplayEffectSpecHandle();
	DashEndLocation = FVector::ZeroVector;

	bDashFinished = false;
	bDashSynced = false;
	bRecastInputReleased = false;
	bReturnRequested = false;
	bReturnStarted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
