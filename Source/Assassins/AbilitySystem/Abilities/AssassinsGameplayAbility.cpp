// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Teams/AssassinsTeamSubsystem.h"

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
    if (CurrentActorInfo)
    {
        if (CurrentActorInfo->AvatarActor.IsValid())
        {
            if (APawn* Pawn = Cast<APawn>(CurrentActorInfo->AvatarActor.Get()))
            {
                return Pawn->GetController();
            }
        }
    }
    return nullptr;
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

FActiveGameplayEffectHandle UAssassinsGameplayAbility::ApplyGameplayEffectSpecToTargetActor(const FGameplayEffectSpecHandle& SpecHandle, AActor* TargetActor)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    check(TargetASC);

    return ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
}

bool UAssassinsGameplayAbility::IsValidEnemy(AActor* TargetActor) const
{
    AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
    check(AvatarCharacter);

    if (AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(TargetActor))
    {
        return TargetCharacter->GetGenericTeamId() != AvatarCharacter->GetGenericTeamId();
    }

    return false;
}
