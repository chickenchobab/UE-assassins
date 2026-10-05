#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_Ability3.h"

#include "Character/AssassinsCharacter.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "CollisionQueryParams.h"
#include "NativeGameplayTags.h"

namespace ZedShadowSlash
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY1, "Status.Combo.Ability1");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY3, "Status.Combo.Ability3");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_ABILITY3, "Status.Target.Zed.Ability3");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_SLASH, "GameplayCue.Champion.Zed.Slash");

	static constexpr double DamageTraceHeight = 100.0;
};

UGA_Zed_Ability3::UGA_Zed_Ability3(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayTag UGA_Zed_Ability3::GetComboTag() const
{
	return ZedShadowSlash::TAG_STATUS_COMBO_ABILITY3;
}

FGameplayTag UGA_Zed_Ability3::GetPartnerComboTag() const
{
	return ZedShadowSlash::TAG_STATUS_COMBO_ABILITY1;
}

void UGA_Zed_Ability3::PrePlayMontage()
{
	// Super를 부르지 않는다. 커서로 돌지 않고 주변을 벤다.

	BeginCombo();

	K2_ExecuteGameplayCue(ZedShadowSlash::TAG_GAMEPLAYCUE_SLASH, FGameplayEffectContextHandle());
}

void UGA_Zed_Ability3::PostPlayMontage()
{
	SlashAround(GetAvatarActorFromActorInfo());

	WaitToImitate();
}

void UGA_Zed_Ability3::ImitateWith(AAssassinsZedShadow& Shadow)
{
	Shadow.CallShadowSlash();
	SlashAround(&Shadow);
}

void UGA_Zed_Ability3::SlashAround(AActor* SourceActor)
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

void UGA_Zed_Ability3::ApplyDamageAndStatus(AActor* HitActor)
{
	const AAssassinsCharacter* HitCharacter = Cast<AAssassinsCharacter>(HitActor);
	if (HitCharacter && HitCharacter->HasMatchingGameplayTag(ZedShadowSlash::TAG_STATUS_TARGET_ABILITY3))
	{
		return;
	}

	ApplyEffectToTarget(DamageEffectClass, HitActor);
	ApplyEffectToTarget(SlashTargetEffectClass, HitActor);
}
