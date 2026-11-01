#include "AbilitySystem/Abilities/Champions/Akali/Ability_Akali_Q.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsCharacter.h"
#include "CollisionQueryParams.h"
#include "NativeGameplayTags.h"

namespace AkaliFivePointStrike
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY1, "Status.Combo.Q");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ANIM_NOTIFY_STATE_END, "Event.AnimNotifyState.End");

	static constexpr EAbilityCustomReplicatedEvent StrikeEvent = EAbilityCustomReplicatedEvent::GameCustom1;

	static constexpr double DamageTraceHeight = 100.0;
};

UAbility_Akali_Q::UAbility_Akali_Q(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbility_Akali_Q::PrePlayMontage()
{
	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->FreezeRotation();
	}
	AddTagToAvatar(AssassinsGameplayTags::Status_Channeling);

	Super::PrePlayMontage();

	AddTagToAvatar(AkaliFivePointStrike::TAG_STATUS_COMBO_ABILITY1);
}

void UAbility_Akali_Q::PostPlayMontage()
{
	if (IsLocallyControlled())
	{
		UAbilityTask_WaitGameplayEvent* NotifyTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, AkaliFivePointStrike::TAG_EVENT_ANIM_NOTIFY_STATE_END, nullptr, true, true);
		NotifyTask->EventReceived.AddDynamic(this, &ThisClass::OnStrikeNotify);
		NotifyTask->ReadyForActivation();
		return;
	}

	if (K2_HasAuthority())
	{
		UAbilityTask_WaitReplicatedEvent* StrikeTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliFivePointStrike::StrikeEvent);
		StrikeTask->OnEvent.AddDynamic(this, &ThisClass::OnStrikeRequested);
		StrikeTask->ReadyForActivation();
	}
}

void UAbility_Akali_Q::OnStrikeNotify(FGameplayEventData Payload)
{
	SendPredictedEventToServer(AkaliFivePointStrike::StrikeEvent, [this]() { Strike(); });
}

void UAbility_Akali_Q::OnStrikeRequested()
{
	Strike();
}

void UAbility_Akali_Q::Strike()
{
	RemoveTagFromAvatar(AkaliFivePointStrike::TAG_STATUS_COMBO_ABILITY1);
	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);

	ApplyDamageInSector();
}

void UAbility_Akali_Q::ApplyDamageInSector()
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

bool UAbility_Akali_Q::IsWithinSectorAngle(const FVector& ImpactPoint) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return false;
	}

	const FVector ToImpactPoint = (ImpactPoint - AvatarActor->GetActorLocation()).GetSafeNormal2D(1.e-4);
	const double Cosine = FMath::Clamp(FVector::DotProduct(AvatarActor->GetActorForwardVector(), ToImpactPoint), -1.0, 1.0);

	return FMath::RadiansToDegrees(FMath::Acos(Cosine)) <= SectorHalfAngleDegree;
}
