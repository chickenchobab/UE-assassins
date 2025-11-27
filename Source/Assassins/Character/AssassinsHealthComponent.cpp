// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AssassinsHealthComponent.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "AssassinsLogCategories.h"


UAssassinsHealthComponent::UAssassinsHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
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

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void UAssassinsHealthComponent::UninitializeFromAbilitySystem()
{
	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void UAssassinsHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	UE_LOG(LogTemp, Display, TEXT("Health changed [%f] -> [%f]"), OldValue, NewValue);
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UAssassinsHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	UE_LOG(LogTemp, Display, TEXT("You died."));
}
