// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Executions/GameplayEffectExecutionCalculation_BlueprintBase.h"
#include "AbilitySystemComponent.h"

UAbilitySystemComponent* UGameplayEffectExecutionCalculation_BlueprintBase::GetTargetAbilitySystemComponentFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const
{
	return Parameters.GetTargetAbilitySystemComponent();
}

UAbilitySystemComponent* UGameplayEffectExecutionCalculation_BlueprintBase::GetSourceAbilitySystemComponentFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const
{
	return Parameters.GetSourceAbilitySystemComponent();
}

AActor* UGameplayEffectExecutionCalculation_BlueprintBase::GetTargetAvatarActorFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const
{
	if (UAbilitySystemComponent* TargetASC = Parameters.GetTargetAbilitySystemComponent())
	{
		return TargetASC->GetAvatarActor();
	}
	return nullptr;
}

AActor* UGameplayEffectExecutionCalculation_BlueprintBase::GetSourceAvatarActorFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const
{
	if (UAbilitySystemComponent* SourceASC = Parameters.GetSourceAbilitySystemComponent())
	{
		return SourceASC->GetAvatarActor();
	}
	return nullptr;
}

FGameplayEffectContextHandle UGameplayEffectExecutionCalculation_BlueprintBase::GetEffectContextFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const
{
	return Parameters.GetOwningSpec().GetEffectContext();
}
