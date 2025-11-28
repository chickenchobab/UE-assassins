// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Executions/AssassinsAttackDamageExecution.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"

struct FDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition AttackDamageDef;
	FGameplayEffectAttributeCaptureDefinition AbilityPowerDef;

	FGameplayEffectAttributeCaptureDefinition TargetArmorDef;
	FGameplayEffectAttributeCaptureDefinition TargetMagicResistanceDef;

	FGameplayEffectAttributeCaptureDefinition ArmorPenetrationPercentDef;
	FGameplayEffectAttributeCaptureDefinition ArmorPenetrationFlatDef;
	FGameplayEffectAttributeCaptureDefinition MagicPenetrationPercentDef;
	FGameplayEffectAttributeCaptureDefinition MagicPenetrationFlatDef;

	FDamageStatics()
	{
		AttackDamageDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetAttackDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		AbilityPowerDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetAbilityPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);

		TargetArmorDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, true);
		TargetMagicResistanceDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetMagicResistanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, true);

		ArmorPenetrationPercentDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetArmorPenetrationPercentAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		ArmorPenetrationFlatDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetArmorPenetrationFlatAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		MagicPenetrationPercentDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetMagicPenetrationPercentAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		MagicPenetrationFlatDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetMagicPenetrationFlatAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	}
};

static FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}

UAssassinsAttackDamageExecution::UAssassinsAttackDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().AbilityPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().TargetArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().TargetMagicResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationPercentDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationFlatDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicPenetrationPercentDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicPenetrationFlatDef);
}

void UAssassinsAttackDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	
	// Me: Lyra uses a typed context to get damage attenuation info from the source object(weapon)
	FGameplayEffectContext* EffectContext = Spec.GetContext().Get();
	check(EffectContext);

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// Me: Get attributes to calculate the attack damage.
	float AttackDamage = 0.0f;
	float TargetArmor = 0.0f;
	float ArmorPenetrationFlat = 0.0f;
	float ArmorPenetrationPercent = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDamageDef, EvaluateParameters, AttackDamage);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().TargetArmorDef, EvaluateParameters, TargetArmor);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationFlatDef, EvaluateParameters, ArmorPenetrationFlat);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationPercentDef, EvaluateParameters, ArmorPenetrationPercent);

	const float TargetArmorDone = FMath::Max(TargetArmor * ((100.f - ArmorPenetrationPercent) / 100.f) - ArmorPenetrationFlat, 0.f);

	const float DamageAttenuation = AttackDamage * (TargetArmorDone / (TargetArmorDone + 100.f));

	// Clamping is done when damage is converted to -health
	const float DamageDone = FMath::Max(AttackDamage - DamageAttenuation, 0.0f);

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
