// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityTestChecks.h"
#include "AbilityTestSession.h"

#include "AbilitySystem/Attributes/AssassinsHealthSet.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "UObject/UnrealType.h"

FAbilityTestRecorder::FAbilityTestRecorder(UAbilitySystemComponent* InAbilitySystem, TSubclassOf<UGameplayAbility> InAbilityClass)
	: AbilitySystem(InAbilitySystem)
	, AbilityClass(InAbilityClass)
{
	if (InAbilitySystem == nullptr)
	{
		return;
	}

	ActivatedHandle = InAbilitySystem->AbilityActivatedCallbacks.AddLambda([this](UGameplayAbility* Ability)
	{
		if (IsRecorded(Ability))
		{
			if (NumActivations == 0)
			{
				FirstActivationTime = FPlatformTime::Seconds();
			}
			++NumActivations;
		}
	});

	EndedHandle = InAbilitySystem->AbilityEndedCallbacks.AddLambda([this](UGameplayAbility* Ability)
	{
		if (IsRecorded(Ability))
		{
			++NumEnds;
			LastEndTime = FPlatformTime::Seconds();
		}
	});

	FailedHandle = InAbilitySystem->AbilityFailedCallbacks.AddLambda([this](const UGameplayAbility* Ability, const FGameplayTagContainer& FailureTags)
	{
		if (IsRecorded(Ability))
		{
			Failures += FString::Printf(TEXT("%sactivation failed(%s)"), Failures.IsEmpty() ? TEXT("") : TEXT(", "), FailureTags.IsEmpty() ? TEXT("no reason given") : *FailureTags.ToStringSimple());
		}
	});
}

FAbilityTestRecorder::~FAbilityTestRecorder()
{
	if (UAbilitySystemComponent* ASC = AbilitySystem.Get())
	{
		ASC->AbilityActivatedCallbacks.Remove(ActivatedHandle);
		ASC->AbilityEndedCallbacks.Remove(EndedHandle);
		ASC->AbilityFailedCallbacks.Remove(FailedHandle);
	}
}

bool FAbilityTestRecorder::IsActive() const
{
	const UAbilitySystemComponent* ASC = AbilitySystem.Get();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromClass(AbilityClass) : nullptr;
	return Spec && Spec->IsActive();
}

bool FAbilityTestRecorder::IsRecorded(const UGameplayAbility* Ability) const
{
	return Ability && AbilityClass && Ability->GetClass()->IsChildOf(AbilityClass);
}

namespace AbilityTestDamageLog
{
	// Effects this close together landed with the same hit.
	constexpr double SameHitSeconds = 0.05;
}

FAbilityTestDamageLog::FAbilityTestDamageLog(UAbilitySystemComponent* InAbilitySystem)
	: AbilitySystem(InAbilitySystem)
	, StartTime(FPlatformTime::Seconds())
{
	if (InAbilitySystem == nullptr)
	{
		return;
	}

	HealthChangedHandle = InAbilitySystem->GetGameplayAttributeValueChangeDelegate(UAssassinsHealthSet::GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		if (Data.NewValue < Data.OldValue)
		{
			PendingDamage += Data.OldValue - Data.NewValue;
		}
	});

	AppliedHandle = InAbilitySystem->OnGameplayEffectAppliedDelegateToSelf.AddLambda([this](UAbilitySystemComponent*, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle)
	{
		Add(Spec);
	});

	PeriodicHandle = InAbilitySystem->OnPeriodicGameplayEffectExecuteDelegateOnSelf.AddLambda([this](UAbilitySystemComponent*, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle)
	{
		Add(Spec);
	});
}

FAbilityTestDamageLog::~FAbilityTestDamageLog()
{
	if (UAbilitySystemComponent* ASC = AbilitySystem.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UAssassinsHealthSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		ASC->OnGameplayEffectAppliedDelegateToSelf.Remove(AppliedHandle);
		ASC->OnPeriodicGameplayEffectExecuteDelegateOnSelf.Remove(PeriodicHandle);
	}
}

void FAbilityTestDamageLog::Add(const FGameplayEffectSpec& Spec)
{
	const UGameplayAbility* Ability = Spec.GetContext().GetAbility();

	FEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.Time = FPlatformTime::Seconds();
	Entry.Ability = Ability ? Ability->GetClass()->GetName() : FString();
	Entry.Effect = Spec.Def ? Spec.Def->GetClass()->GetName() : FString();
	Entry.Damage = PendingDamage;
	PendingDamage = 0.f;
}

float FAbilityTestDamageLog::GetDamage(const FString& AbilityClassName, double SinceTime) const
{
	float Damage = 0.f;
	for (const FEntry& Entry : Entries)
	{
		if ((Entry.Time >= SinceTime) && (AbilityClassName.IsEmpty() || (Entry.Ability == AbilityClassName)))
		{
			Damage += Entry.Damage;
		}
	}
	return Damage;
}

int32 FAbilityTestDamageLog::CountHits(const FString& AbilityClassName, double SinceTime) const
{
	int32 Hits = 0;
	double LastHitTime = -1.0;
	for (const FEntry& Entry : Entries)
	{
		if ((Entry.Time < SinceTime) || (Entry.Ability != AbilityClassName) || (Entry.Damage <= 0.f))
		{
			continue;
		}

		if ((LastHitTime < 0.0) || (Entry.Time - LastHitTime > AbilityTestDamageLog::SameHitSeconds))
		{
			++Hits;
		}
		LastHitTime = Entry.Time;
	}
	return Hits;
}

FString FAbilityTestDamageLog::DescribeByAbility() const
{
	TArray<TPair<FString, float>> ByAbility;
	for (const FEntry& Entry : Entries)
	{
		if (Entry.Damage <= 0.f)
		{
			continue;
		}

		const FString Name = Entry.Ability.IsEmpty() ? FString(TEXT("(no ability)")) : Entry.Ability;
		TPair<FString, float>* Found = ByAbility.FindByPredicate([&Name](const TPair<FString, float>& Pair) { return Pair.Key == Name; });
		if (Found)
		{
			Found->Value += Entry.Damage;
		}
		else
		{
			ByAbility.Emplace(Name, Entry.Damage);
		}
	}

	TArray<FString> Parts;
	for (const TPair<FString, float>& Pair : ByAbility)
	{
		Parts.Add(FString::Printf(TEXT("%s %.1f"), *Pair.Key, Pair.Value));
	}
	return Parts.IsEmpty() ? FString(TEXT("none")) : FString::Join(Parts, TEXT(", "));
}

FString FAbilityTestDamageLog::Describe() const
{
	TArray<FString> Lines;
	for (const FEntry& Entry : Entries)
	{
		Lines.Add(FString::Printf(TEXT("  %6.2fs %s / %s: %.1f"), Entry.Time - StartTime,
			Entry.Ability.IsEmpty() ? TEXT("(no ability)") : *Entry.Ability, *Entry.Effect, Entry.Damage));
	}
	return FString::Join(Lines, TEXT("\n"));
}

namespace AbilityTestChecks
{
	float GetHealth(const UAbilitySystemComponent* AbilitySystem)
	{
		return AbilitySystem ? AbilitySystem->GetNumericAttribute(UAssassinsHealthSet::GetHealthAttribute()) : 0.f;
	}

	bool HasCooldownEffect(const UGameplayAbility* AbilityCDO)
	{
		const FGameplayTagContainer* CooldownTags = AbilityCDO ? AbilityCDO->GetCooldownTags() : nullptr;
		return CooldownTags && !CooldownTags->IsEmpty();
	}

	bool IsOnCooldown(const UAbilitySystemComponent* AbilitySystem, const UGameplayAbility* AbilityCDO)
	{
		const FGameplayTagContainer* CooldownTags = AbilityCDO ? AbilityCDO->GetCooldownTags() : nullptr;
		return AbilitySystem && CooldownTags && !CooldownTags->IsEmpty() && AbilitySystem->HasAnyMatchingGameplayTags(*CooldownTags);
	}

	FString FindDamageEffects(const UGameplayAbility* AbilityCDO)
	{
		if (AbilityCDO == nullptr)
		{
			return FString();
		}

		TArray<FString> DamageEffects;
		for (TFieldIterator<FClassProperty> It(AbilityCDO->GetClass()); It; ++It)
		{
			const FClassProperty* Property = *It;
			if (!Property->MetaClass || !Property->MetaClass->IsChildOf(UGameplayEffect::StaticClass()) || !Property->GetName().Contains(TEXT("Damage")))
			{
				continue;
			}

			if (const UObject* EffectClass = Property->GetObjectPropertyValue_InContainer(AbilityCDO))
			{
				DamageEffects.Add(FString::Printf(TEXT("%s=%s"), *Property->GetName(), *EffectClass->GetName()));
			}
		}
		return FString::Join(DamageEffects, TEXT(", "));
	}

	FString FindLeftoverTags(const UAbilitySystemComponent* AbilitySystem)
	{
		if (AbilitySystem == nullptr)
		{
			return FString();
		}

		FGameplayTagContainer OwnedTags;
		AbilitySystem->GetOwnedGameplayTags(OwnedTags);

		TArray<FString> Leftovers;
		for (const FGameplayTag& Tag : OwnedTags)
		{
			if (FAbilityTestSession::GetTransientTags().HasTagExact(Tag))
			{
				Leftovers.Add(Tag.ToString());
			}
		}
		return FString::Join(Leftovers, TEXT(", "));
	}
}
