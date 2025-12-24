// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"

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

AController* UAssassinsGameplayAbility::GetControllerFromActorInfo() const
{
    //Me: When an owner chain occurs?
    return (CurrentActorInfo ? CurrentActorInfo->PlayerController.Get() : nullptr);
}

AAssassinsCharacter* UAssassinsGameplayAbility::GetAssassinsCharacterFromActorInfo() const
{
    return Cast<AAssassinsCharacter>(GetAvatarActorFromActorInfo());
}

FGameplayEffectContextHandle UAssassinsGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
    FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

    FGameplayEffectContext* EffectContext = ContextHandle.Get();

    AActor* Instigator = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
    AActor* EffectCauser = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
    UObject* SourceObject = GetSourceObject(Handle, ActorInfo);

    EffectContext->AddInstigator(Instigator, EffectCauser);
    EffectContext->AddSourceObject(SourceObject);

    return ContextHandle;
}

FGameplayEffectSpecHandle UAssassinsGameplayAbility::MakeEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass)
{
    FGameplayEffectContextHandle EffectContext = MakeEffectContext(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo());

    UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponentFromActorInfo();
    check(AssassinsASC);

    return AssassinsASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectContext);
}

void UAssassinsGameplayAbility::SetSetByCallerMagnitudeOfEffectSpec(const FGameplayEffectSpecHandle& SpecHandle, FGameplayTag DataTag, float Magnitude)
{
    if (SpecHandle.IsValid())
    {
        if (SpecHandle.Data.IsValid())
        {
            SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
        }
    }
}

FActiveGameplayEffectHandle UAssassinsGameplayAbility::ApplyGameplayEffectSpecToTargetActor(const FGameplayEffectSpecHandle& SpecHandle, AActor* TargetActor)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    check(TargetASC);

    return ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
}

bool UAssassinsGameplayAbility::DoesActorHaveTag(AActor* Actor, FGameplayTag TagToCheck)
{
    if (UAbilitySystemComponent* ActorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
    {
        return ActorASC->HasMatchingGameplayTag(TagToCheck);
    }
    return false;
}
