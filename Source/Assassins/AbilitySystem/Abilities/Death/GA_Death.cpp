#include "AbilitySystem/Abilities/Death/GA_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Character/AssassinsCharacter.h"
#include "Components/SceneComponent.h"
#include "Teams/AssassinsTeamBaseActor.h"

UGA_Death::UGA_Death(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive())
	{
		PlayDeath();
	}
}

void UGA_Death::PlayDeath()
{
	bMontageEnded = false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && IndicatorComponentClass)
	{
		if (USceneComponent* Indicator = Cast<USceneComponent>(AvatarActor->GetComponentByClass(IndicatorComponentClass)))
		{
			Indicator->SetVisibility(false, true);
		}
	}

	UAnimMontage* DeathMontage = nullptr;
	if (const AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		const TArray<TObjectPtr<UAnimMontage>>& DeathMontages = AvatarCharacter->GetDeathMontages();
		if (!DeathMontages.IsEmpty())
		{
			DeathMontage = DeathMontages[FMath::RandRange(0, DeathMontages.Num() - 1)];
		}
	}
	else if (const AAssassinsTeamBaseActor* TeamBase = Cast<AAssassinsTeamBaseActor>(AvatarActor))
	{
		DeathMontage = TeamBase->DeadMontage;
	}

	if (DeathMontage == nullptr)
	{
		OnMontageEnd();
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage, 1.0f, NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->ReadyForActivation();
}

void UGA_Death::HandleMontageEnded()
{
	if (bMontageEnded)
	{
		return;
	}

	bMontageEnded = true;
	OnMontageEnd();
}

void UGA_Death::OnMontageEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
