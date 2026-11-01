// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AssassinsProjectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Teams/AssassinsTeamAgentInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "AssassinsLogCategories.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace AssassinsProjectile
{
    // 비행이 끝난 투사체가 마지막 이동들의 판정을 기다리는 최대 시간. 판정은 그 프레임 끝에 끝나고(UAssassinsInterframeHitCheckSubsystem::RunPendingHitChecks),
    // 이 값은 끝나지 않는 판정에 대비한 것이다.
    constexpr double FlightOverWaitLimit = 1.0;
}

// 캐릭터는 서버가 프레임마다 남긴 기록과 판정한다(UAssassinsInterframeHitCheckSubsystem, 모드 1). 투사체의 이동마다 그 구간 동안 각
// 캐릭터가 있던 위치와 비교한다. 겹침으로 찾으면 투사체의 두 이동 사이에 지나가는 캐릭터를 놓친다. 명중은 시간 순서로 와서,
// 첫 대상에서 멈추는 투사체는 먼저 닿은 대상을 맞힌다. 캐릭터가 아닌 것(구조물)은 여전히 겹침으로 찾는다.
AAssassinsProjectile::AAssassinsProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);
    
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionBox->SetCollisionResponseToChannel(ECC_GameTraceChannel4/*structure*/, ECR_Overlap);
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AAssassinsProjectile::HandleProjectileBeginOverlap);

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(GetRootComponent());
    StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StaticMesh->SetVisibility(false);

    ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticleSystem"));
    ParticleSystemComponent->SetupAttachment(GetRootComponent());
    ParticleSystemComponent->bAutoActivate = false;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bAutoActivate = false;

    StartTime = 0.0f;
    StartLocation = FVector::Zero();
}

bool AAssassinsProjectile::IsValidTarget(AActor* TargetActor, bool bShouldNotBeInstigator, bool bShouldBeEnemy, bool bCanTargetStructure) const
{
    AAssassinsCharacter* InstigatorCharacter = Cast<AAssassinsCharacter>(GetInstigator());
    if (InstigatorCharacter == nullptr)
    {
        UE_LOG(LogAssassins, Error, TEXT("%s has no character instigator: spawn it with the champion as its instigator."), *GetNameSafe(this));
        return false;
    }

    const IAssassinsTeamAgentInterface* TargetTeamAgent = Cast<IAssassinsTeamAgentInterface>(TargetActor);
    if (TargetTeamAgent == nullptr)
    {
        return false;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    if (TargetASC == nullptr)
    {
        return false;
    }

    bool bValidTarget = true;

    if (bShouldNotBeInstigator)
    {
        bValidTarget &= (TargetActor != InstigatorCharacter);
    }

    if (bShouldBeEnemy)
    {
        bValidTarget &= (TargetTeamAgent->GetGenericTeamId() != InstigatorCharacter->GetGenericTeamId());
    }

    if (!bCanTargetStructure)
    {
        bValidTarget &= !TargetASC->HasMatchingGameplayTag(AssassinsGameplayTags::Structure);
    }

    return bValidTarget;
}

void AAssassinsProjectile::EnableAndSetDistanceRange(float NewDistanceRange)
{
    bUseDistanceRange = true;
    DistanceRange = NewDistanceRange;
}

void AAssassinsProjectile::EnableAndSetLifeSpan(float NewLifeSpan)
{
    bUseLifeSpan = true;
    ProjectileLifeSpan = NewLifeSpan;
}

void AAssassinsProjectile::BeginPlay()
{
	Super::BeginPlay();

    // 서버: 캐릭터는 겹침이 아니라 각자의 이동에 대해 판정한다. 비행마다 한 번 정한다.
    if (HasAuthority() && UAssassinsInterframeHitCheckSubsystem::UsesServerRecords() && (UAssassinsInterframeHitCheckSubsystem::Get(this) != nullptr))
    {
        bChecksCharactersOnRecords = true;
        CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }

    // 이동이 끝난 뒤 그 이동을 판정하도록 무브먼트 틱 뒤에 틱한다.
    PrimaryActorTick.AddPrerequisite(ProjectileMovement, ProjectileMovement->PrimaryComponentTick);

    if (ActivationDelay > 0.0)
    {
        GetWorldTimerManager().SetTimer(ActivationTimerHandle, this, &ThisClass::ActivateProjectile, static_cast<float>(ActivationDelay), false);
    }
    else
    {
        ActivateProjectile();
    }

    if (bUseLifeSpan)
    {
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::EndFlight, ProjectileLifeSpan, false);
    }

    StartLocation = GetActorLocation();
}

void AAssassinsProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HitCheckHandle != 0)
    {
        if (UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this))
        {
            HitCheck->EndProjectileHitChecks(HitCheckHandle);
        }
        HitCheckHandle = 0;
    }

    Super::EndPlay(EndPlayReason);
}

void AAssassinsProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bFlightOver)
    {
        const UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this);
        const bool bSettled = (HitCheck == nullptr) || !HitCheck->HasPendingHitChecks(HitCheckHandle);
        if (bSettled || (GetWorld()->GetTimeSeconds() - FlightOverTime > AssassinsProjectile::FlightOverWaitLimit))
        {
            Destroy();
        }
        return;
    }

    CheckFlightHits(DeltaTime);

    if (bUseDistanceRange)
    {
        const float MovedDistance = FVector::Dist2D(GetActorLocation(), StartLocation);

        if (MovedDistance >= DistanceRange)
        {
            EndFlight();
        }
    }
}

void AAssassinsProjectile::ActivateProjectile()
{
    bActivated = true;

    ProjectileMovement->Activate(false);
    ParticleSystemComponent->Activate(false);
    StaticMesh->SetVisibility(true, true);

    if (IsInitialized)
    {
        StartHandlingHits();
    }
}

void AAssassinsProjectile::FinishInitProjectile_Implementation()
{
    IsInitialized = true;

    if (bActivated)
    {
        StartHandlingHits();
    }
}

void AAssassinsProjectile::StartHandlingHits()
{
    if (bHandlingHits || !HasAuthority())
    {
        return;
    }
    bHandlingHits = true;

    // 여기부터 이동마다 캐릭터의 이동과 판정한다. 이미 닿아 있는 캐릭터는 첫 이동이 시작하면서 찾는다.
    UAssassinsInterframeHitCheckSubsystem* HitCheck = bChecksCharactersOnRecords ? UAssassinsInterframeHitCheckSubsystem::Get(this) : nullptr;
    if (HitCheck != nullptr)
    {
        const TWeakObjectPtr<ThisClass> WeakThis(this);
        HitCheckHandle = HitCheck->BeginProjectileHitChecks(this, GetHitShape(),
            [WeakThis](AActor* Target)
            {
                const ThisClass* Self = WeakThis.Get();
                return (Self != nullptr) && !Self->ShouldBeDestroyed && !Self->HandledActors.Contains(Target) && Self->CanHitCheckTarget(Target);
            },
            [WeakThis](AActor* Target)
            {
                if (ThisClass* Self = WeakThis.Get())
                {
                    Self->HandleCheckedHit(Target);
                }
            });
        LastCheckedTime = GetWorld()->GetTimeSeconds();
        LastCheckedLocation = GetActorLocation();
    }

    // 이미 겹친 것을 하나씩 처리하다 하나가 끝내면 멈춘다. 판정 대상인 캐릭터는 겹침으로 찾지 않는다.
    TArray<AActor*> OverlappingActors;
    CollisionBox->GetOverlappingActors(OverlappingActors);
    for (AActor* OverlappingActor : OverlappingActors)
    {
        HandleHit(OverlappingActor, OverlappingActor->GetActorLocation());
        if (ShouldBeDestroyed)
        {
            break;
        }
    }
}

void AAssassinsProjectile::HandleHit(AActor* TargetActor, const FVector& ImpactLocation)
{
    // 액터마다 한 번, 처리할 때만 넣는다. 날기 전에 겹쳤던 대상은 닿을 때 맞는다.
    if ((TargetActor == nullptr) || HandledActors.Contains(TargetActor))
    {
        return;
    }

    HandledActors.Add(TargetActor);
    HandleTargetOverlap(TargetActor, ImpactLocation);

    // 명중으로 끝나면(첫 대상에서 멈춤) 이동들이 맞히려던 나머지는 버린다.
    if (ShouldBeDestroyed && (HitCheckHandle != 0))
    {
        if (UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this))
        {
            HitCheck->CancelHitChecks(HitCheckHandle);
        }
    }
}

bool AAssassinsProjectile::CanHitCheckTarget(AActor* TargetActor) const
{
    return IsValidTarget(TargetActor);
}

FAssassinsInterframeHitShape AAssassinsProjectile::GetHitShape() const
{
    const FVector Extent = CollisionBox->GetScaledBoxExtent();

    FAssassinsInterframeHitShape Shape;
    Shape.Radius = static_cast<float>(FMath::Max(Extent.X, Extent.Y));
    Shape.HalfHeight = static_cast<float>(FMath::Max(Extent.Z, static_cast<double>(Shape.Radius)));
    return Shape;
}

void AAssassinsProjectile::CheckFlightHits(float DeltaTime)
{
    if ((HitCheckHandle == 0) || ShouldBeDestroyed)
    {
        return;
    }

    UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this);
    if (HitCheck == nullptr)
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();

    // 이번 프레임의 이동은 프레임 전체에 걸친다. 그 전에는 제자리에 있었다(생성된 프레임에는 틱하지 않는다).
    const double MoveStartTime = FMath::Max(LastCheckedTime, Now - DeltaTime);
    if (MoveStartTime > LastCheckedTime)
    {
        HitCheck->CheckProjectileSpanHit(HitCheckHandle, LastCheckedTime, LastCheckedLocation, MoveStartTime, LastCheckedLocation);
        LastCheckedTime = MoveStartTime;
        if ((HitCheckHandle == 0) || ShouldBeDestroyed)
        {
            return;
        }
    }

    double EndTime = Now;
    FVector End = GetActorLocation();

    if (bUseDistanceRange)
    {
        const double StartDistance = FVector::Dist2D(LastCheckedLocation, StartLocation);
        const double EndDistance = FVector::Dist2D(End, StartLocation);
        if ((EndDistance > DistanceRange) && (EndDistance > StartDistance))
        {
            const double Fraction = FMath::Clamp((DistanceRange - StartDistance) / (EndDistance - StartDistance), 0.0, 1.0);
            End = FMath::Lerp(LastCheckedLocation, End, Fraction);
            EndTime = FMath::Lerp(LastCheckedTime, EndTime, Fraction);
        }
    }

    if (EndTime <= LastCheckedTime)
    {
        return;
    }

    const double SpanStartTime = LastCheckedTime;
    const FVector SpanStart = LastCheckedLocation;
    LastCheckedTime = EndTime;
    LastCheckedLocation = End;
    HitCheck->CheckProjectileSpanHit(HitCheckHandle, SpanStartTime, SpanStart, EndTime, End);
}

void AAssassinsProjectile::HandleCheckedHit(AActor* TargetActor)
{
    // 명중을 넘길 때 대상이 선 위치를 쓴다. 투사체는 캐릭터 명중의 위치를 쓰지 않는다.
    if (!ShouldBeDestroyed && IsValid(TargetActor))
    {
        HandleHit(TargetActor, TargetActor->GetActorLocation());
    }
}

void AAssassinsProjectile::EndFlight()
{
    if (bFlightOver)
    {
        return;
    }

    const UAssassinsInterframeHitCheckSubsystem* HitCheck = (HitCheckHandle != 0) ? UAssassinsInterframeHitCheckSubsystem::Get(this) : nullptr;
    if ((HitCheck == nullptr) || !HitCheck->HasPendingHitChecks(HitCheckHandle))
    {
        Destroy();
        return;
    }

    bFlightOver = true;
    FlightOverTime = GetWorld()->GetTimeSeconds();
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->Deactivate();
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void AAssassinsProjectile::SetVelocity(const FVector& NewVelocity)
{
    if (NewVelocity.IsNearlyZero())
    {
        return;
    }

    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = NewVelocity;
        ProjectileMovement->MaxSpeed = NewVelocity.Size();
    }
}

void AAssassinsProjectile::LaunchStraight(const FVector& Velocity, float Range, const FGameplayEffectSpecHandle& Damage)
{
    DamageSpecHandle = Damage;
    SetVelocity(Velocity);
    EnableAndSetDistanceRange(Range);
    FinishInitProjectile();
}

void AAssassinsProjectile::EnableHoming(USceneComponent* TargetComponent, float HomingAcceleration)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->bIsHomingProjectile = true;
        ProjectileMovement->HomingTargetComponent = TargetComponent;
        ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
    }
}

void AAssassinsProjectile::DisableHoming()
{
    if (ProjectileMovement)
    {
        ProjectileMovement->bIsHomingProjectile = false;
        ProjectileMovement->HomingTargetComponent = nullptr;
        ProjectileMovement->HomingAccelerationMagnitude = 0.0f;
    }
}

void AAssassinsProjectile::HandleProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bHandlingHits)
    {
        HandleHit(OtherActor, SweepResult.ImpactPoint);
    }
}

void AAssassinsProjectile::ApplyGameplayEffectSpecToTargetActor(const FGameplayEffectSpecHandle& SpecHandle, AActor* TargetActor)
{
    AAssassinsCharacter* InstigatorCharacter = Cast<AAssassinsCharacter>(GetInstigator());
    if (InstigatorCharacter == nullptr)
    {
        UE_LOG(LogAssassins, Error, TEXT("%s has no character instigator: spawn it with the champion as its instigator."), *GetNameSafe(this));
        return;
    }

    UAbilitySystemComponent* SourceASC = InstigatorCharacter->GetAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

    if (SourceASC && TargetASC && SpecHandle.Data.IsValid())
    {
        SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
    }
}
