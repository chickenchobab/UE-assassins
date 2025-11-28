// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Executions/AssassinsDamageExecution.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"

struct FDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;

	FDamageStatics()
	{
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	}
};

static FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}

UAssassinsDamageExecution::UAssassinsDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().BaseDamageDef);
}

void UAssassinsDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	
	// Me: Lyra uses a typed context to 
	FGameplayEffectContext* EffectContext = Spec.GetContext().Get();
	check(EffectContext);

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDamageDef, EvaluateParameters, BaseDamage);

	// Clamping is done when damage is converted to -health
	const float DamageDone = FMath::Max(BaseDamage, 0.0f);

	if (DamageDone > 0.0f)
	{
		// Apply a damage modifier, this gets turned into -health on the target.
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UAssassinsHealthSet::GetDamageAttribute(),
				EGameplayModOp::Additive,
				DamageDone
			)
		);
	}
}
