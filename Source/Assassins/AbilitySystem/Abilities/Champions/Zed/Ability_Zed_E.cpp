#include "AbilitySystem/Abilities/Champions/Zed/Ability_Zed_E.h"

#include "Character/AssassinsCharacter.h"
#include "Character/Champions/Zed/ZedShadow.h"
#include "CollisionQueryParams.h"
#include "NativeGameplayTags.h"

namespace ZedShadowSlash
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY1, "Status.Combo.Q");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY3, "Status.Combo.E");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_ABILITY3, "Status.Target.Zed.E");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_SLASH, "GameplayCue.Champion.Zed.Slash");

	static constexpr double DamageTraceHeight = 100.0;
};

UAbility_Zed_E::UAbility_Zed_E(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayTag UAbility_Zed_E::GetComboTag() const
{
	return ZedShadowSlash::TAG_STATUS_COMBO_ABILITY3;
}

FGameplayTag UAbility_Zed_E::GetPartnerComboTag() const
{
	return ZedShadowSlash::TAG_STATUS_COMBO_ABILITY1;
}

void UAbility_Zed_E::PrePlayMontage()
{
	// Super를 부르지 않는다. 커서로 돌지 않고 주변을 벤다.

	BeginCombo();

	K2_ExecuteGameplayCue(ZedShadowSlash::TAG_GAMEPLAYCUE_SLASH, FGameplayEffectContextHandle());
}

void UAbility_Zed_E::PostPlayMontage()
{
	SlashAround(GetAvatarActorFromActorInfo());

	WaitToImitate();
}

void UAbility_Zed_E::ImitateWith(AZedShadow& Shadow)
{
	Shadow.CallShadowSlash();
	SlashAround(&Shadow);
}

void UAbility_Zed_E::SlashAround(AActor* SourceActor)
{
	if (!IsValid(SourceActor))
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	for (const TEnumAsByte<EObjectTypeQuery>& ObjectType : TypesToSphereTrace)
	{
		ObjectQueryParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjectType));
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	const double TopZ = SourceLocation.Z + ZedShadowSlash::DamageTraceHeight;
	for (const FHitResult& Hit : SweepForEnemies(SourceLocation, EffectRadius, TopZ, ObjectQueryParams, SourceActor))
	{
		ApplyDamageAndStatus(Hit.GetActor());
	}
}

void UAbility_Zed_E::ApplyDamageAndStatus(AActor* HitActor)
{
	const AAssassinsCharacter* HitCharacter = Cast<AAssassinsCharacter>(HitActor);
	if (HitCharacter && HitCharacter->HasMatchingGameplayTag(ZedShadowSlash::TAG_STATUS_TARGET_ABILITY3))
	{
		return;
	}

	ApplyEffectToTarget(DamageEffectClass, HitActor);
	ApplyEffectToTarget(SlashTargetEffectClass, HitActor);
}
