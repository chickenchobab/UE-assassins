// Fill out your copyright notice in the Description page of Project Settings.


#include "Physics/AssassinsProjectile.h"
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
    CollisionBox->OnComponentHit.AddUniqueDynamic(this, &AAssassinsProjectile::HandleProjectileHit);
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

bool AAssassinsProjectile::IsValidTarget(AActor* TargetActor) const
{
    if (TargetActor->IsA<AAssassinsCharacter>())
    {
        if (GetOwner() && GetOwner() != TargetActor)
        {
            return true;
        }
    }

    return false;
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

void AAssassinsProjectile::HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
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
    AAssassinsCharacter* OwnerCharacter = Cast<AAssassinsCharacter>(GetOwner());
    check(OwnerCharacter);

    if (AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(TargetActor))
    {
        if (UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent())
        {
            ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetCharacter->GetAbilitySystemComponent());
        }
    }
}
