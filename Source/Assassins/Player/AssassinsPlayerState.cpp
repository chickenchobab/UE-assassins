// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AssassinsPlayerState.h"

#include "Player/AssassinsPlayerController.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"

AAssassinsPlayerController* AAssassinsPlayerState::GetAssassinsPlayerController() const
{
    return Cast<AAssassinsPlayerController>(GetOwner());
}

UAbilitySystemComponent* AAssassinsPlayerState::GetAbilitySystemComponent() const
{ 
    return GetAssassinsAbilitySystemComponent();
}
