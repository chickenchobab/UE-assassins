// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AbilitySystem/AssassinsTargetChasingComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Character/AssassinsHeroComponent.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Player/AssassinsPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Teams/AssassinsTeamAgentInterface.h"
#include "Teams/AssassinsTeamSubsystem.h"
#include "Animation/AssassinsAnimInstance.h"
#include "AssassinsGameplayTags.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DEATH, "Status.Death");

UAssassinsGameplayAbility::UAssassinsGameplayAbility(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    ActivationPolicy = EAssassinsAbilityActivationPolicy::OnInputTriggered;

    bCanTargetStructure = false;
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

void UAssassinsGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const
{
    // Try activate the ability on spawn.
    if (ActorInfo && !Spec.IsActive() && (ActivationPolicy == EAssassinsAbilityActivationPolicy::OnSpawn))
    {
        UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

        // If avatar actor is torn off or about to die, don't try to activate until we get the new one.
        if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
        {
            const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
            const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

            const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
            const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

            if (bClientShouldActivate || bServerShouldActivate)
            {
                ASC->TryActivateAbility(Spec.Handle);
            }
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

    // The target may be gone by the time an ability applies to it, e.g. when it died while the ability waited.
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    if ((TargetASC == nullptr) || !SpecHandle.IsValid())
    {
        return FActiveGameplayEffectHandle();
    }

    return ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC, ASC->ScopedPredictionKey);
}

FActiveGameplayEffectHandle UAssassinsGameplayAbility::ApplyEffectToTarget(TSubclassOf<UGameplayEffect> EffectClass, AActor* TargetActor)
{
    return ApplyGameplayEffectSpecToTargetActor(MakeEffectSpecHandle(EffectClass), TargetActor);
}

AAssassinsProjectile* UAssassinsGameplayAbility::SpawnAbilityProjectile(TSubclassOf<AAssassinsProjectile> ProjectileClass, const FTransform& SpawnTransform, AActor* ProjectileOwner) const
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    UWorld* World = GetWorld();
    if ((AvatarActor == nullptr) || (World == nullptr) || !ProjectileClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParameters.Owner = ProjectileOwner ? ProjectileOwner : AvatarActor;
    SpawnParameters.Instigator = Cast<APawn>(AvatarActor);

    return World->SpawnActor<AAssassinsProjectile>(ProjectileClass, SpawnTransform, SpawnParameters);
}

TArray<FHitResult> UAssassinsGameplayAbility::SweepForEnemies(const FVector& Center, double Radius, double TopZ, const FCollisionObjectQueryParams& ObjectQueryParams, const AActor* SourceActor) const
{
    TArray<FHitResult> EnemyHits;

    UWorld* World = GetWorld();
    if ((World == nullptr) || !ObjectQueryParams.IsValid())
    {
        return EnemyHits;
    }

    const FVector Start(Center.X, Center.Y, 0.0);
    const FVector End(Center.X, Center.Y, TopZ);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AbilitySweepForEnemies), /*bTraceComplex*/ false, GetAvatarActorFromActorInfo());
    QueryParams.AddIgnoredActor(SourceActor);

    TArray<FHitResult> Hits;
    World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectQueryParams, FCollisionShape::MakeSphere(static_cast<float>(Radius)), QueryParams);

    TSet<const AActor*> FoundActors;
    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!FoundActors.Contains(HitActor) && IsValidEnemy(HitActor))
        {
            FoundActors.Add(HitActor);
            EnemyHits.Add(Hit);
        }
    }
    return EnemyHits;
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
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    if (TargetASC == nullptr)
    {
        return false;
    }

    if (TargetASC->HasMatchingGameplayTag(TAG_DEATH))
    {
        return false;
    }

    // Me: Basic attacks opt in to hitting structures, every other ability leaves the flag off.
    if (!bCanTargetStructure && TargetASC->HasMatchingGameplayTag(AssassinsGameplayTags::Structure))
    {
        return false;
    }

    const IAssassinsTeamAgentInterface* TargetTeamAgent = Cast<IAssassinsTeamAgentInterface>(TargetActor);
    const IAssassinsTeamAgentInterface* AvatarTeamAgent = Cast<IAssassinsTeamAgentInterface>(GetAvatarActorFromActorInfo());
    if ((TargetTeamAgent == nullptr) || (AvatarTeamAgent == nullptr))
    {
        return false;
    }

    return TargetTeamAgent->GetGenericTeamId() != AvatarTeamAgent->GetGenericTeamId();
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

void UAssassinsGameplayAbility::AddCancelledByTag(FGameplayTag Tag)
{
    CancelledByTags.AddTag(Tag);
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

EAbilityGenericReplicatedEvent::Type UAssassinsGameplayAbility::ToGenericReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent)
{
    return static_cast<EAbilityGenericReplicatedEvent::Type>(
        static_cast<uint8>(EAbilityGenericReplicatedEvent::GameCustom1) + static_cast<uint8>(CustomEvent));
}

void UAssassinsGameplayAbility::ServerSetReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    // The event goes with a prediction key of its own: the server handles it inside a window of that key.
    FScopedPredictionWindow ScopedPrediction(ASC, IsPredictingClient());

    ASC->ServerSetReplicatedEvent(ToGenericReplicatedEvent(CustomEvent), GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey(), ASC->ScopedPredictionKey);
}

void UAssassinsGameplayAbility::ClientSetReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    ASC->ClientSetReplicatedEvent(ToGenericReplicatedEvent(CustomEvent), GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey());
}

void UAssassinsGameplayAbility::CallOrAddReplicatedDelegate(EAbilityCustomReplicatedEvent CustomEvent, FAbilityReplicatedDelegate ReplicatedDelegate, bool bUnbindCalledDelegate)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    check(ASC);

    const EAbilityGenericReplicatedEvent::Type EventType = ToGenericReplicatedEvent(CustomEvent);
    const FGameplayAbilitySpecHandle SpecHandle = GetCurrentAbilitySpecHandle();
    const FPredictionKey ActivationKey = GetCurrentActivationInfo().GetActivationPredictionKey();

    // Only this event is consumed and only this delegate is unbound: the other events and signals the activation
    // received(e.g. the one a sync point waits for next) are left alone.
    TSharedRef<FDelegateHandle> DelegateHandle = MakeShared<FDelegateHandle>();
    FSimpleMulticastDelegate::FDelegate EventDelegate = FSimpleMulticastDelegate::FDelegate::CreateWeakLambda(this,
        [this, ReplicatedDelegate, bUnbindCalledDelegate, EventType, SpecHandle, ActivationKey, DelegateHandle]()
        {
            if (bUnbindCalledDelegate)
            {
                if (UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo())
                {
                    OwnerASC->AbilityReplicatedEventDelegate(EventType, SpecHandle, ActivationKey).Remove(*DelegateHandle);
                    OwnerASC->ConsumeGenericReplicatedEvent(EventType, SpecHandle, ActivationKey);
                }
            }

            ReplicatedDelegate.ExecuteIfBound();
        });
    *DelegateHandle = EventDelegate.GetHandle();

    // Runs the delegate right away when the event arrived before anyone listened, adds it otherwise.
    ASC->CallOrAddReplicatedDelegate(EventType, SpecHandle, ActivationKey, EventDelegate);
}

void UAssassinsGameplayAbility::SendPredictedEventToServer(EAbilityCustomReplicatedEvent CustomEvent, TFunctionRef<void()> LocalAction)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC == nullptr)
    {
        return;
    }

    FScopedPredictionWindow ScopedPrediction(ASC, IsPredictingClient());

    // Sent first, so that the server hears of it even when LocalAction ends the ability.
    if (IsPredictingClient())
    {
        ASC->ServerSetReplicatedEvent(ToGenericReplicatedEvent(CustomEvent), GetCurrentAbilitySpecHandle(), GetCurrentActivationInfo().GetActivationPredictionKey(), ASC->ScopedPredictionKey);
    }

    LocalAction();
}