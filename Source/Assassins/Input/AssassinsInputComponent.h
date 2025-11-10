// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnhancedInputComponent.h"
#include "Input/AssassinsInputConfig.h"
#include "AssassinsLogCategories.h"

#include "AssassinsInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;


/**
 * 
 */
UCLASS(Config = Input)
class UAssassinsInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	
	void AddInputMappings(const UAssassinsInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	void RemoveInputMappings(const UAssassinsInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

	template<class UserClass, typename FuncType>
	void BindNativeAction(const UAssassinsInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UAssassinsInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	void RemoveBinds(TArray<uint32>& BindHandles);
};

template<class UserClass, typename FuncType>
void UAssassinsInputComponent::BindNativeAction(const UAssassinsInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
{
	check(InputConfig);

	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UAssassinsInputComponent::BindAbilityActions(const UAssassinsInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);

	for (const FAssassinsInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
				UE_LOG(LogAssassins, Log, TEXT("Valid pressed function."));
			}
			else
			{
				UE_LOG(LogAssassins, Error, TEXT("Invalid processed function."));
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
				UE_LOG(LogAssassins, Log, TEXT("Valid released function."));
			}
			else
			{
				UE_LOG(LogAssassins, Error, TEXT("Invalid released function."));
			}
		}
	}
}