// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/AssassinsTargetChasingComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Character/AssassinsHeroComponent.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Player/AssassinsPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Teams/AssassinsTeamSubsystem.h"
#include "Animation/AssassinsAnimInstance.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DEATH, "Status.Death");

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

    CancelledByTags.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UAssassinsTargetChasingComponent* UAssassinsGameplayAbility::GetTargetChasingComponentFromController() const
{
    if (AController* C = GetControllerFromActorInfo())
    {
        return C->FindComponentByClass<UAssassinsTargetChasingComponent>();
    }

    return nullptr;
}

AActor* UAssassinsGameplayAbility::GetCurrentCursorTarget() const
{
    if (UAssassinsAbilitySystemComponent* ASC = GetAssassinsAbilitySystemComponentFromActorInfo())
    {
        return ASC->GetCursorTargetFromHeroComponent();
    }

    return nullptr;
}

bool UAssassinsGameplayAbility::IsInputTriggered() const
{
    return GetAssassinsAbilitySystemComponentFromActorInfo()->IsCurrentEventAbilityInput();
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
        if (TargetCharacter->HasGameplayTag(TAG_DEATH))
        {
            return false;
        }
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

const FGameplayTagContainer& UAssassinsGameplayAbility::K2_GetAssetTags() const
{
    // Used to identify the inheritance hierarchy between abilities.
    return GetAssetTags();
}

void UAssassinsGameplayAbility::AddCancelledByTag(FGameplayTag Tag)
{
    CancelledByTags.AddTag(Tag);
}

void UAssassinsGameplayAbility::RemoveCancelledByTag(FGameplayTag Tag)
{
    if (!CancelledByTags.HasTag(Tag))
    {
        return;
    }

    CancelledByTags.RemoveTag(Tag);
}

void UAssassinsGameplayAbility::SetAvatarLocationAndRotation(const FVector& GoalLocation, const FRotator& GoalRotation)
{
    ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    check(AvatarCharacter);

    if (UAssassinsCharacterMovementComponent* AssassinsCMC = Cast<UAssassinsCharacterMovementComponent>(AvatarCharacter->GetCharacterMovement()))
    {
        AssassinsCMC->TeleportCharacter(GoalLocation, GoalRotation);
    }
}

void UAssassinsGameplayAbility::ServerSetReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    // TODO scoped window

    EAbilityGenericReplicatedEvent::Type EventType = static_cast<EAbilityGenericReplicatedEvent::Type>(
        static_cast<uint8>(EAbilityGenericReplicatedEvent::GameCustom1) + static_cast<uint8>(CustomEvent)
        );

    ASC->ServerSetReplicatedEvent(EventType, GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey(), ASC->ScopedPredictionKey);
}

void UAssassinsGameplayAbility::ClientSetReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    EAbilityGenericReplicatedEvent::Type EventType = static_cast<EAbilityGenericReplicatedEvent::Type>(
        static_cast<uint8>(EAbilityGenericReplicatedEvent::GameCustom1) + static_cast<uint8>(CustomEvent)
        );

    ASC->ClientSetReplicatedEvent(EventType, GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey());
}

void UAssassinsGameplayAbility::CallOrAddReplicatedDelegate(EAbilityCustomReplicatedEvent CustomEvent, FAbilityReplicatedDelegate ReplicatedDelegate, bool bClearTargetData)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    EAbilityGenericReplicatedEvent::Type EventType = static_cast<EAbilityGenericReplicatedEvent::Type>(
        static_cast<uint8>(EAbilityGenericReplicatedEvent::GameCustom1) + static_cast<uint8>(CustomEvent)
        );

    ASC->CallOrAddReplicatedDelegate(
        EventType,
        GetCurrentAbilitySpecHandle(), 
        GetCurrentActivationInfo().GetActivationPredictionKey(), 
        FSimpleMulticastDelegate::FDelegate::CreateLambda([this, ReplicatedDelegate, bClearTargetData]() {
            if (bClearTargetData)
            {
                if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
                {
                    ASC->ClearAbilityReplicatedDataCache(GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo());
                }
            }

            ReplicatedDelegate.ExecuteIfBound(); 
        })
    );
}