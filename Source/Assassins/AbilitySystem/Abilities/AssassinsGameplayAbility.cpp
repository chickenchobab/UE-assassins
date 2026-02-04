// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Teams/AssassinsTeamSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AssassinsAnimInstance.h"

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

UAssassinsAnimInstance* UAssassinsGameplayAbility::GetAssassinsAnimInstanceFromActorInfo() const
{
    if (CurrentActorInfo)
    {
        return Cast<UAssassinsAnimInstance>(CurrentActorInfo->GetAnimInstance());
    }
    return nullptr;
}

void UAssassinsGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnGiveAbility(ActorInfo, Spec);

    TryActivateAbilityOnSpawn(ActorInfo, Spec);
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

void UAssassinsGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTags(AvatarStatusTags);
    }
    AvatarStatusTags.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAssassinsGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const
{
    // Try activate the ability on spawn.
    if (ActorInfo && !Spec.IsActive() && (ActivationPolicy == EAssassinsAbilityActivationPolicy::OnSpawn))
    {
        UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

        // If avatar actor is torn off or about to die, don't try to activate until we get the new one.
        if (ASC && AvatarActor && (AvatarActor->GetLifeSpan() <= 0.0f))
        {
            ASC->TryActivateAbility(Spec.Handle);
        }
    }
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

float UAssassinsGameplayAbility::EvaluateCurveTableRowByAbilityLevel(UCurveTable* CurveTable, FName RowName, const FString& ContextString) const
{
    FCurveTableRowHandle Handle;
    Handle.CurveTable = CurveTable;
    Handle.RowName = RowName;

    float ReturnValue;
    if (Handle.Eval(GetAbilityLevel(), &ReturnValue, ContextString))
    {
        return ReturnValue;
    }
    
    return 0.0f;
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

void UAssassinsGameplayAbility::AddTagToAvatar(FGameplayTag Tag)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(Tag);
        AvatarStatusTags.AddTag(Tag);
    }
}

void UAssassinsGameplayAbility::RemoveTagFromAvatar(FGameplayTag Tag)
{
    if (!AvatarStatusTags.HasTag(Tag))
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(Tag);
        AvatarStatusTags.RemoveTag(Tag);
    }
}

