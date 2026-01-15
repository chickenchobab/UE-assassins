// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AssassinsHealthComponent.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "AssassinsLogCategories.h"
#include "AssassinsGameplayTags.h"

UAssassinsHealthComponent::UAssassinsHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	DeathState = EAssassinsDeathState::NotDead;
}

void UAssassinsHealthComponent::InitializeWithAbilitySystem(UAssassinsAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogAssassins, Error, TEXT("AssassinsHealthComponent: Health component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogAssassins, Error, TEXT("AssassinsHealthComponent: Cannot initialize health component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<UAssassinsHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogAssassins, Error, TEXT("AssassinsHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);

	// Reset attributes to default values. Eventually this will be driven by a spread sheet.
	AbilitySystemComponent->SetNumericAttributeBase(UAssassinsHealthSet::GetHealthAttribute(), HealthSet->GetMaxHealth());

	ClearGameplayTags();

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void UAssassinsHealthComponent::UninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

float UAssassinsHealthComponent::GetHealthNormalized() const
{
    if (HealthSet)
    {
        const float Health = HealthSet->GetHealth();
        const float MaxHealth = HealthSet->GetMaxHealth();

        return (MaxHealth > 0.0f ? (Health / MaxHealth) : 0.0f);
    }

    return 0.0f;
}

void UAssassinsHealthComponent::StartDeath()
{
	if (DeathState != EAssassinsDeathState::NotDead)
	{
		return;
	}

	DeathState = EAssassinsDeathState::DeathStarted;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(AssassinsGameplayTags::Status_Death_Dying, 1);
	}

	OnDeathStarted.Broadcast();
}

void UAssassinsHealthComponent::FinishDeath()
{
	if (DeathState != EAssassinsDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EAssassinsDeathState::DeathFinished;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(AssassinsGameplayTags::Status_Death_Dead, 1);
	}

	OnDeathFinished.Broadcast();
}

void UAssassinsHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(AssassinsGameplayTags::Status_Death_Dying, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(AssassinsGameplayTags::Status_Death_Dead, 0);
	}
}

void UAssassinsHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	UE_LOG(LogTemp, Display, TEXT("Health changed [%f] -> [%f]"), OldValue, NewValue);
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UAssassinsHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent && DamageEffectSpec)
	{
		// Send the "Event.Death" gameplay event through the owner's ability system.
		// This can be used to trigger a death gameplay ability.
		FGameplayEventData Payload;
		Payload.EventTag = AssassinsGameplayTags::Event_Death;
		Payload.Instigator = DamageInstigator;
		Payload.Target = AbilitySystemComponent->GetAvatarActor();
		Payload.OptionalObject = DamageEffectSpec->Def;
		Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
		Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
		Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
		Payload.EventMagnitude = DamageMagnitude;

		FScopedPredictionWindow NewScopeWindow(AbilitySystemComponent, true);
		AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
#endif
}
