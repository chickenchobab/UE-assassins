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

    UFUNCTION(BlueprintPure, Category = "Assassins|Projectile")
    virtual bool IsValidTarget(AActor* TargetActor, bool bShouldNotBeInstigator=true, bool bShouldBeEnemy=true, bool bCanTargetStructure=false) const;

    UFUNCTION(BlueprintPure)
    UParticleSystemComponent* GetProjectileParticle() const { return ParticleSystemComponent; }

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void EnableAndSetDistanceRange(float NewDistanceRange);

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void EnableAndSetLifeSpan(float NewLifeSpan);

    // Public for the abilities that launch the projectile, as the blueprints could already call it.
    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void SetVelocity(const FVector& NewVelocity);

    // Called by the ability once the projectile is set up, so that it starts handling what it overlaps.
    // Implemented by B_Projectile_Base.
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Projectile")
    void FinishInitProjectile();

    // Makes the projectile chase the target. Implemented by B_ProjectileWithTarget.
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Projectile")
    void InitHomingProjectile(AActor* ProjectileTarget, const FGameplayEffectSpecHandle& DamageContextHandle, double HomingAcceleration);

    // Sends the projectile straight on at Velocity, for Range at most, with the damage it applies to what it hits, and has
    // it start handling what it overlaps(FinishInitProjectile). Whatever else it needs, e.g. a handler of its end, is set
    // before: it may hit something right away.
    void LaunchStraight(const FVector& Velocity, float Range, const FGameplayEffectSpecHandle& Damage);

    // What the projectile does when it reaches a target. Implemented by B_Projectile_Base and the projectiles overriding it.
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Projectile")
    void HandleTargetOverlap(AActor* TargetActor, const FVector& ImpactLocation);

public:

    // Damage to apply on hit, made by the ability which spawned the projectile.
    UPROPERTY(BlueprintReadWrite, Category = "Assassins|Projectile")
    FGameplayEffectSpecHandle DamageSpecHandle;

    // Damage to apply on the first hit only(Zed Ability1).
    UPROPERTY(BlueprintReadWrite, Category = "Assassins|Projectile")
    FGameplayEffectSpecHandle DamageSpecHandle_FirstHit;

protected:
	
    //~AActor interface
	virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    //~End of AActor interface

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void EnableHoming(USceneComponent* TargetComponent, float HomingAcceleration);
    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void DisableHoming();

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
