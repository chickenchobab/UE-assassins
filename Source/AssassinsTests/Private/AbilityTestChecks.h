// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayEffectSpec;

/**
 * FAbilityTestRecorder
 *
 * Counts the activations and ends of an ability on an ability system as they happen, so that an ability that starts and
 * ends between two looks is still seen. Also keeps why its activations failed, for the error message.
 */
class FAbilityTestRecorder
{
public:
	FAbilityTestRecorder(UAbilitySystemComponent* InAbilitySystem, TSubclassOf<UGameplayAbility> InAbilityClass);
	~FAbilityTestRecorder();

	FAbilityTestRecorder(const FAbilityTestRecorder&) = delete;
	FAbilityTestRecorder& operator=(const FAbilityTestRecorder&) = delete;

	int32 GetNumActivations() const { return NumActivations; }
	int32 GetNumEnds() const { return NumEnds; }

	// FPlatformTime::Seconds() of the first activation and of the last end, 0 before there is one.
	double GetFirstActivationTime() const { return FirstActivationTime; }
	double GetLastEndTime() const { return LastEndTime; }

	// Why the activations failed, as the ability system told it. Empty when none did.
	const FString& GetFailures() const { return Failures; }

	// Whether the ability is running now.
	bool IsActive() const;

private:
	bool IsRecorded(const UGameplayAbility* Ability) const;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	TSubclassOf<UGameplayAbility> AbilityClass;

	int32 NumActivations = 0;
	int32 NumEnds = 0;
	double FirstActivationTime = 0.0;
	double LastEndTime = 0.0;
	FString Failures;

	FDelegateHandle ActivatedHandle;
	FDelegateHandle EndedHandle;
	FDelegateHandle FailedHandle;
};

/**
 * FAbilityTestDamageLog
 *
 * Writes down the effects an ability system takes, with the ability that sent each and the health it cost. An instant
 * effect executes, then the ability system tells it was applied: the health lost in between goes to that effect. Tells
 * which ability of a combo dealt which damage.
 */
class FAbilityTestDamageLog
{
public:
	struct FEntry
	{
		// FPlatformTime::Seconds()
		double Time = 0.0;

		// The class names of the ability that sent the effect(empty when none did) and of the effect.
		FString Ability;
		FString Effect;

		float Damage = 0.f;
	};

	explicit FAbilityTestDamageLog(UAbilitySystemComponent* InAbilitySystem);
	~FAbilityTestDamageLog();

	FAbilityTestDamageLog(const FAbilityTestDamageLog&) = delete;
	FAbilityTestDamageLog& operator=(const FAbilityTestDamageLog&) = delete;

	const TArray<FEntry>& GetEntries() const { return Entries; }

	// The damage the ability(by its class name, every ability when empty) dealt from the time on.
	float GetDamage(const FString& AbilityClassName = FString(), double SinceTime = 0.0) const;

	// How many times the ability dealt damage from the time on. Effects that land together count as one hit.
	int32 CountHits(const FString& AbilityClassName, double SinceTime = 0.0) const;

	// The damage by ability, in the order they first dealt some: "GA_Zed_Ability1_C 596.4, GA_Zed_Ability3_C 220.4".
	FString DescribeByAbility() const;

	// Every effect with its time from the start of the log, for the log file.
	FString Describe() const;

private:
	void Add(const FGameplayEffectSpec& Spec);

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	double StartTime = 0.0;

	// Lost since the last effect, for the next one.
	float PendingDamage = 0.f;

	TArray<FEntry> Entries;

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle AppliedHandle;
	FDelegateHandle PeriodicHandle;
};

/** The checks the ability tests share. */
namespace AbilityTestChecks
{
	float GetHealth(const UAbilitySystemComponent* AbilitySystem);

	// Whether the ability has a cooldown effect.
	bool HasCooldownEffect(const UGameplayAbility* AbilityCDO);

	// Whether the ability's cooldown is on the ability system.
	bool IsOnCooldown(const UAbilitySystemComponent* AbilitySystem, const UGameplayAbility* AbilityCDO);

	// The damage effects the ability is set up with: its gameplay effect class properties named ...Damage..., when set.
	// Empty when it has none, e.g. an ability that deals its damage through another one(Leblanc's mimic).
	FString FindDamageEffects(const UGameplayAbility* AbilityCDO);

	// The transient tags(FAbilityTestSession::GetTransientTags) still on the ability system, empty when none are.
	FString FindLeftoverTags(const UAbilitySystemComponent* AbilitySystem);
}
