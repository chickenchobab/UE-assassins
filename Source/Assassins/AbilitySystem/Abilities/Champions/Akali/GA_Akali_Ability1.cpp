// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Ability1.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsCharacter.h"
#include "CollisionQueryParams.h"
#include "NativeGameplayTags.h"

namespace AkaliFivePointStrike
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY1, "Status.Combo.Ability1");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ANIM_NOTIFY_STATE_END, "Event.AnimNotifyState.End");

	// The owning client tells the server it strikes.
	static constexpr EAbilityCustomReplicatedEvent StrikeEvent = EAbilityCustomReplicatedEvent::GameCustom1;

	// The strike is a column from the ground up to this height above Akali.
	static constexpr double DamageTraceHeight = 100.0;
};

UGA_Akali_Ability1::UGA_Akali_Ability1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Akali_Ability1::PrePlayMontage()
{
	// Akali keeps facing the way she throws. The channeling tag gives the rotation back when it is over.
	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->FreezeRotation();
	}
	AddTagToAvatar(AssassinsGameplayTags::Status_Channeling);

	// Faces the cursor.
	Super::PrePlayMontage();

	AddTagToAvatar(AkaliFivePointStrike::TAG_STATUS_COMBO_ABILITY1);
}

void UGA_Akali_Ability1::PostPlayMontage()
{
	if (IsLocallyControlled())
	{
		UAbilityTask_WaitGameplayEvent* NotifyTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, AkaliFivePointStrike::TAG_EVENT_ANIM_NOTIFY_STATE_END, /*OptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
		NotifyTask->EventReceived.AddDynamic(this, &ThisClass::OnStrikeNotify);
		NotifyTask->ReadyForActivation();
		return;
	}

	// The server strikes when the owning client says it did.
	if (K2_HasAuthority())
	{
		UAbilityTask_WaitReplicatedEvent* StrikeTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliFivePointStrike::StrikeEvent);
		StrikeTask->OnEvent.AddDynamic(this, &ThisClass::OnStrikeRequested);
		StrikeTask->ReadyForActivation();
	}
}

void UGA_Akali_Ability1::OnStrikeNotify(FGameplayEventData Payload)
{
	SendPredictedEventToServer(AkaliFivePointStrike::StrikeEvent, [this]() { Strike(); });
}

void UGA_Akali_Ability1::OnStrikeRequested()
{
	Strike();
}

void UGA_Akali_Ability1::Strike()
{
	RemoveTagFromAvatar(AkaliFivePointStrike::TAG_STATUS_COMBO_ABILITY1);
	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);

	ApplyDamageInSector();
}

void UGA_Akali_Ability1::ApplyDamageInSector()
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return;
	}

	const FGameplayEffectSpecHandle DamageSpecHandle = MakeEffectSpecHandle(DamageEffectClass);

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const double TopZ = AvatarLocation.Z + AkaliFivePointStrike::DamageTraceHeight;
	for (const FHitResult& Hit : SweepForEnemies(AvatarLocation, EffectRadius, TopZ, FCollisionObjectQueryParams(ECC_Pawn)))
	{
		if (IsWithinSectorAngle(Hit.ImpactPoint))
		{
			ApplyGameplayEffectSpecToTargetActor(DamageSpecHandle, Hit.GetActor());
		}
	}
}

bool UGA_Akali_Ability1::IsWithinSectorAngle(const FVector& ImpactPoint) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return false;
	}

	// Only the XY plane matters.
	const FVector ToImpactPoint = (ImpactPoint - AvatarActor->GetActorLocation()).GetSafeNormal2D(1.e-4);
	const double Cosine = FMath::Clamp(FVector::DotProduct(AvatarActor->GetActorForwardVector(), ToImpactPoint), -1.0, 1.0);

	return FMath::RadiansToDegrees(FMath::Acos(Cosine)) <= SectorHalfAngleDegree;
}
