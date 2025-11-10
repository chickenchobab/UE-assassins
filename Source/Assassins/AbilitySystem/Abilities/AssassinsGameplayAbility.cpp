// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerController.h"

UAssassinsGameplayAbility::UAssassinsGameplayAbility(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    ActivationPolicy = EAssassinsAbilityActivationPolicy::OnInputTriggered;
}

UAssassinsAbilitySystemComponent* UAssassinsGameplayAbility::GetAssassinsAbilitySystemComponentFromActorInfo() const
{
    return Cast<UAssassinsAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

AAssassinsPlayerController* UAssassinsGameplayAbility::GetAssassinsPlayerControllerFromActorInfo() const
{
    return (CurrentActorInfo ? Cast<AAssassinsPlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

AAssassinsCharacter* UAssassinsGameplayAbility::GetAssassinsCharacterFromActorInfo() const
{
    return Cast<AAssassinsCharacter>(GetAvatarActorFromActorInfo());
}
