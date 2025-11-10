// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/AssassinsInputConfig.h"

const UInputAction* UAssassinsInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FAssassinsInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}

const UInputAction* UAssassinsInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FAssassinsInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}
