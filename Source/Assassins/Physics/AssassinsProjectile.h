// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "GameplayEffectTypes.h"
#include "AssassinsProjectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
struct FGameplayEffectSpecHandle;

UCLASS()
class ASSASSINS_API AAssassinsProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AAssassinsProjectile();

    UFUNCTION(BlueprintPure)
    virtual bool IsValidTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintPure)
    UParticleSystemComponent* GetProjectileParticle() const { return ParticleSystemComponent; }

protected:
	
	virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void EnableHoming(USceneComponent* TargetComponent, float HomingAcceleration);

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void DisableHoming();

    UFUNCTION()
    void HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    UFUNCTION(BlueprintImplementableEvent, Category = "Assassins|Projectile", DisplayName = "HandleProjectileHit")
    void K2_HandleProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void HandleProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION(BlueprintImplementableEvent, Category = "Assassins|Projectile", DisplayName = "HandleProjectileBeginOverlap")
    void K2_HandleProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void ApplyGameplayEffectSpecToTargetActor(const FGameplayEffectSpecHandle& SpecHandle, AActor* TargetActor);

protected:

    /////////////////////////
    // Components
    /////////////////////////

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UBoxComponent* CollisionBox;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UStaticMeshComponent* StaticMesh;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UParticleSystemComponent* ParticleSystemComponent;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    UProjectileMovementComponent* ProjectileMovement;

    /////////////////////////////
    // Projectile configurations
    /////////////////////////////

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile", Meta = (AllowPrivateAccess = "true"))
    bool bUseDistanceRange;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile", Meta = (EditCondition = "bUseDistanceRange", AllowPrivateAccess = "true"))
    float DistanceRange;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile", Meta = (AllowPrivateAccess = "true"))
    bool bUseLifeSpan;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile", Meta = (EditCondition = "bUseLifeSpan", AllowPrivateAccess = "true"))
    float ProjectileLifeSpan;

private:

    float StartTime;

    FVector StartLocation;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ActorsToIgnore;
};
