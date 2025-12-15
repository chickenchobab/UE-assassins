// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "GameplayEffectTypes.h"
#include "AssassinsProjectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;

UCLASS()
class ASSASSINS_API AAssassinsProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AAssassinsProjectile();

    UFUNCTION(BlueprintPure)
    virtual bool IsValidTarget(AActor* TargetActor) const;

protected:
	
	virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    UFUNCTION(BlueprintImplementableEvent, Category = "Assassins|Projectile", DisplayName = "HandleProjectileHit")
    void K2_HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void HandleProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION(BlueprintImplementableEvent, Category = "Assassins|Projectile", DisplayName = "HandleProjectileBeginOverlap")
    void K2_HandleProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UBoxComponent* CollisionBox;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UStaticMeshComponent* StaticMesh;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UParticleSystemComponent* ParticleSystemComponent;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UProjectileMovementComponent* ProjectileMovement;

private:

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile", Meta = (AllowPrivateAccess = "true"))
    float DistanceRange;

    FVector StartLocation;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ActorsToIgnore;
};
