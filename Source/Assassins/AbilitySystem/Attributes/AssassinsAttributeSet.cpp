// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/AssassinsAttributeSet.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"

UAssassinsAbilitySystemComponent* UAssassinsAttributeSet::GetAssassinsAbilitySystemComponent() const
{
	return Cast<UAssassinsAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}