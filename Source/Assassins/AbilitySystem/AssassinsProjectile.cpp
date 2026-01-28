// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AssassinsProjectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Character/AssassinsCharacter.h"
#include "AbilitySystemComponent.h"

AAssassinsProjectile::AAssassinsProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
    
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AAssassinsProjectile::HandleProjectileBeginOverlap);

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(GetRootComponent());
    StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticleSystem"));
    ParticleSystemComponent->SetupAttachment(GetRootComponent());

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    StartTime = 0.0f;
    StartLocation = FVector::Zero();
}

bool AAssassinsProjectile::IsValidTarget(AActor* TargetActor, bool bShouldNotBeInstigator, bool bShouldBeEnemy) const
{
    AAssassinsCharacter* InstigatorCharacter = Cast<AAssassinsCharacter>(GetInstigator());
    check(InstigatorCharacter);

    AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(TargetActor);
    if (TargetCharacter == nullptr)
    {
        return false;
    }
    
    bool bValidTarget = true;

    if (bShouldNotBeInstigator)
    {
        bValidTarget &= (TargetCharacter != InstigatorCharacter);
    }

    if (bShouldBeEnemy)
    {
        bValidTarget &= (TargetCharacter->GetGenericTeamId() != InstigatorCharacter->GetGenericTeamId());
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

    if (bUseLifeSpan)
    {
        FTimerHandle TimerHandle;

        GetWorldTimerManager().SetTimer(
            TimerHandle,
            [this]() { Destroy(); },
            ProjectileLifeSpan,
            false
        );
    }

    StartLocation = GetActorLocation();
}

void AAssassinsProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bUseDistanceRange)
    {
        const float MovedDistance = FVector::Dist2D(GetActorLocation(), StartLocation);

        if (MovedDistance >= DistanceRange)
        {
            Destroy();
        }
    }
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
    if (OtherActor == nullptr)
    {
        return;
    }

    if (ActorsToIgnore.Contains(OtherActor))
    {
        return;
    }

    ActorsToIgnore.Add(OtherActor);

    K2_HandleProjectileBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AAssassinsProjectile::ApplyGameplayEffectSpecToTargetActor(const FGameplayEffectSpecHandle& SpecHandle, AActor* TargetActor)
{
    AAssassinsCharacter* InstigatorCharacter = Cast<AAssassinsCharacter>(GetInstigator());
    check(InstigatorCharacter);

    if (AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(TargetActor))
    {
        if (UAbilitySystemComponent* ASC = InstigatorCharacter->GetAbilitySystemComponent())
        {
            if (SpecHandle.Data.IsValid())
            {
                ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetCharacter->GetAbilitySystemComponent());
            }
        }
    }
}
