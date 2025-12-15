// Fill out your copyright notice in the Description page of Project Settings.


#include "Physics/AssassinsProjectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Character/AssassinsCharacter.h"

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

    StartLocation = GetActorLocation();
}

void AAssassinsProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const float MovedDistance = FVector::Dist2D(GetActorLocation(), StartLocation);

    if (MovedDistance >= DistanceRange)
    {
        Destroy();
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
