// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_Ability2.h"

#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Character/AssassinsCharacter.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

namespace ZedShadowAbility
{
	// Set on the owning side while the shadow is on its way, so that Ability1 and Ability3 wait for it.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_SPAWNING_SHADOW, "Status.Champion.Zed.SpawningShadow");
};

UGA_Zed_Ability2::UGA_Zed_Ability2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Zed_Ability2::PrePlayMontage()
{
	// Not calling Super: Zed does not turn to the cursor for this one.
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && CastParticle)
	{
		UGameplayStatics::SpawnEmitterAttached(CastParticle, AvatarActor->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			FVector::OneVector, EAttachLocation::SnapToTarget, /*bAutoDestroy*/ true);
	}
}

void UGA_Zed_Ability2::PostPlayMontage()
{
	if (IsLocallyControlled())
	{
		AddTagToAvatar(ZedShadowAbility::TAG_STATUS_SPAWNING_SHADOW);
	}
	else
	{
		// The server listens for the recast right away: the client, ahead of it, may swap before the server's shadow
		// is there. Its press then comes in before its end of the ability(same channel, in order).
		WaitForRecast();
	}

	// The shadow appears once it would have travelled to the cursor.
	UWorld* World = GetWorld();
	const double ProjectionTime = GetShadowProjectionTime(CursorLocationClamped);
	if ((World != nullptr) && (ProjectionTime > 0.0))
	{
		World->GetTimerManager().SetTimer(ShadowSpawnTimerHandle, this, &ThisClass::SpawnShadow, static_cast<float>(ProjectionTime), /*bLoop*/ false);
	}
	else
	{
		SpawnShadow();
	}
}

void UGA_Zed_Ability2::SpawnShadow()
{
	if (IsValid(SpawnedShadow))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(ShadowSpawnTimerHandle);
	}

	AAssassinsZedShadow* Shadow = AAssassinsZedShadow::SpawnShadow(ShadowClass, GetAvatarActorFromActorInfo(), FTransform(CursorLookAtRotation, CursorLocationClamped), ShadowLifeSpan);
	if (Shadow == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	SpawnedShadow = Shadow;
	Shadow->OnEndPlay.AddDynamic(this, &ThisClass::OnShadowEndPlay);

	if (IsLocallyControlled())
	{
		RemoveTagFromAvatar(ZedShadowAbility::TAG_STATUS_SPAWNING_SHADOW);
		WaitForRecast();
	}
}

void UGA_Zed_Ability2::WaitForRecast()
{
	// Both sides wait for the input: the client's press reaches the server with its prediction key.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Zed_Ability2::OnRecastReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, /*bTestAlreadyPressed*/ true);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Zed_Ability2::OnRecastPressed(float TimeWaited)
{
	// The server may not have its shadow yet: the client that swapped already had one, so it appears now.
	if (!IsValid(SpawnedShadow))
	{
		SpawnShadow();
	}

	MoveToShadow();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Zed_Ability2::MoveToShadow()
{
	if (IsValid(SpawnedShadow))
	{
		SpawnedShadow->SwapPlacesWith(GetAssassinsCharacterFromActorInfo());
	}
}

void UGA_Zed_Ability2::OnShadowEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	// Only this activation's shadow counts.
	if (Actor != SpawnedShadow)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

double UGA_Zed_Ability2::GetShadowProjectionTime(const FVector& ProjectionLocation) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return 0.0;
	}

	const double Speed = FMath::Clamp(ShadowProjectionSpeed, 0.001, FMath::Max(ShadowProjectionSpeed, 0.001));
	return FVector::Dist2D(AvatarActor->GetActorLocation(), ProjectionLocation) / Speed;
}

void UGA_Zed_Ability2::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// The shadow outlives the ability: its end must not reach the next activation.
	if (IsValid(SpawnedShadow))
	{
		SpawnedShadow->OnEndPlay.RemoveDynamic(this, &ThisClass::OnShadowEndPlay);
	}
	SpawnedShadow = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
