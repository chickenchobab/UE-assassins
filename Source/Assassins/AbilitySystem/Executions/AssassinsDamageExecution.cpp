// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Executions/AssassinsDamageExecution.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"

struct FDamageStatics
{
    // Me: Damages are populated by calculation modifiers
	FGameplayEffectAttributeCaptureDefinition PhysicalDamageDef;
	FGameplayEffectAttributeCaptureDefinition MagicDamageDef;
    FGameplayEffectAttributeCaptureDefinition TrueDamageDef;

	FGameplayEffectAttributeCaptureDefinition TargetArmorDef;
	FGameplayEffectAttributeCaptureDefinition TargetMagicResistanceDef;

	FGameplayEffectAttributeCaptureDefinition ArmorPenetrationPercentDef;
	FGameplayEffectAttributeCaptureDefinition ArmorPenetrationFlatDef;
	FGameplayEffectAttributeCaptureDefinition MagicPenetrationPercentDef;
	FGameplayEffectAttributeCaptureDefinition MagicPenetrationFlatDef;

	FDamageStatics()
	{
		PhysicalDamageDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetPhysicalDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		MagicDamageDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetMagicDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
        TrueDamageDef = FGameplayEffectAttributeCaptureDefinition(UAssassinsCombatSet::GetTrueDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);

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

UAssassinsDamageExecution::UAssassinsDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicDamageDef);
    RelevantAttributesToCapture.Add(DamageStatics().TrueDamageDef);

	RelevantAttributesToCapture.Add(DamageStatics().TargetArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().TargetMagicResistanceDef);

	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationPercentDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationFlatDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicPenetrationPercentDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicPenetrationFlatDef);
}

void UAssassinsDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	
	// Me: Lyra uses a typed context to get damage attenuation info from the source object(weapon)
	FGameplayEffectContext* EffectContext = Spec.GetContext().Get();
	check(EffectContext);

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// Me: Get attributes to calculate the damage.
	float PhysicalDamage = 0.0f;
    float MagicDamage = 0.0f;
    float TrueDamage = 0.0f;
	float TargetArmor = 0.0f;
    float TargetMagicResistance = 0.0f;
	float ArmorPenetrationPercent = 0.0f;
	float ArmorPenetrationFlat = 0.0f;
    float MagicPenetrationPercent = 0.0f;
    float MagicPenetrationFlat = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalDamageDef, EvaluateParameters, PhysicalDamage);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicDamageDef, EvaluateParameters, MagicDamage);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().TrueDamageDef, EvaluateParameters, TrueDamage);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().TargetArmorDef, EvaluateParameters, TargetArmor);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().TargetMagicResistanceDef, EvaluateParameters, TargetMagicResistance);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationPercentDef, EvaluateParameters, ArmorPenetrationPercent);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationFlatDef, EvaluateParameters, ArmorPenetrationFlat);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicPenetrationPercentDef, EvaluateParameters, MagicPenetrationPercent);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicPenetrationFlatDef, EvaluateParameters, MagicPenetrationFlat);

	const float TargetArmorDone = FMath::Max(TargetArmor * ((100.f - ArmorPenetrationPercent) / 100.f) - ArmorPenetrationFlat, 0.f);
    const float TargetMagicResistanceDone = FMath::Max(TargetMagicResistance * ((100.f - MagicPenetrationPercent) / 100.f) - MagicPenetrationFlat, 0.f);

	const float PhysicalDamageAttenuation = PhysicalDamage * (TargetArmorDone / (TargetArmorDone + 100.f));
    const float MagicDamageAttenuation = MagicDamage * (TargetMagicResistanceDone / (TargetMagicResistanceDone + 100.f));

	// Clamping is done when damage is converted to -health
	const float PhysicalDamageDone = FMath::Max(PhysicalDamage - PhysicalDamageAttenuation, 0.0f);
    const float MagicDamageDone = FMath::Max(MagicDamage - MagicDamageAttenuation, 0.0f);
    const float DamageDone = PhysicalDamageDone + MagicDamageDone;

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
