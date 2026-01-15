// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "AssassinsHealthComponent.generated.h"

class UAssassinsAbilitySystemComponent;
class UAssassinsHealthSet;
struct FGameplayEffectSpec;

// Me: Proxy version of the attribute event delegate to handle attribute changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAssassinsHealth_DeathEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FAssassinsHealth_AttributeChanged, UAssassinsHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

/**
* Defines current state of death
*/
UENUM(BlueprintType)
enum class EAssassinsDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished,
};

/**
 * An actor component used to handle anything related to health
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class ASSASSINS_API UAssassinsHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	UAssassinsHealthComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure)
	static UAssassinsHealthComponent* FindHealthComponent(AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UAssassinsHealthComponent>() : nullptr); }

	UFUNCTION(BlueprintCallable, Category = "Assassins|Health")
	void InitializeWithAbilitySystem(UAssassinsAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Health")
	void UninitializeFromAbilitySystem();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Health")
	EAssassinsDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Assassins|Health", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsDeadOrDying() const { return (DeathState > EAssassinsDeathState::NotDead); }

	// Starts the death sequence for the owner
	virtual void StartDeath();

	// Ends the death sequence for the owner
	virtual void FinishDeath();

public:
	UPROPERTY(BlueprintAssignable)
	FAssassinsHealth_AttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FAssassinsHealth_DeathEvent OnDeathStarted;
	UPROPERTY(BlueprintAssignable)
	FAssassinsHealth_DeathEvent OnDeathFinished;

protected:
	
	void ClearGameplayTags();

	// Me: Functions bound to the delegates of the health set
	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

protected:

	// Me: Ability system copied from the owning character
	UPROPERTY()
	TObjectPtr<UAssassinsAbilitySystemComponent> AbilitySystemComponent;

	// Me: Health set copied from the ASC above
	UPROPERTY()
	TObjectPtr<const UAssassinsHealthSet> HealthSet;

	UPROPERTY()
	EAssassinsDeathState DeathState;
};
