// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/AssassinsInterframeHitCheckSubsystem.h"
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

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void SetVelocity(const FVector& NewVelocity);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Assassins|Projectile")
    void FinishInitProjectile();
    virtual void FinishInitProjectile_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Assassins|Projectile")
    void InitHomingProjectile(AActor* ProjectileTarget, const FGameplayEffectSpecHandle& DamageContextHandle, double HomingAcceleration);
    virtual void InitHomingProjectile_Implementation(AActor* ProjectileTarget, const FGameplayEffectSpecHandle& DamageContextHandle, double HomingAcceleration) {}

    void LaunchStraight(const FVector& Velocity, float Range, const FGameplayEffectSpecHandle& Damage);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Assassins|Projectile")
    void HandleTargetOverlap(AActor* TargetActor, const FVector& ImpactLocation);
    virtual void HandleTargetOverlap_Implementation(AActor* TargetActor, const FVector& ImpactLocation) {}

    virtual bool CanHitCheckTarget(AActor* TargetActor) const;

public:

    UPROPERTY(BlueprintReadWrite, Category = "Assassins|Projectile")
    FGameplayEffectSpecHandle DamageSpecHandle;

    UPROPERTY(BlueprintReadWrite, Category = "Assassins|Projectile")
    FGameplayEffectSpecHandle DamageSpecHandle_FirstHit;

protected:
	
    //~AActor interface
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    //~End of AActor interface

    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void EnableHoming(USceneComponent* TargetComponent, float HomingAcceleration);
    UFUNCTION(BlueprintCallable, Category = "Assassins|Projectile")
    void DisableHoming();

    UFUNCTION()
    void HandleProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Projectile")
    double ActivationDelay = 0.0;

    UPROPERTY(BlueprintReadWrite, Category = "Assassins|Projectile")
    bool ShouldBeDestroyed = false;

    UPROPERTY()
    bool IsInitialized = false;

private:

    void ActivateProjectile();

    void StartHandlingHits();

    void HandleHit(AActor* TargetActor, const FVector& ImpactLocation);

    FAssassinsInterframeHitShape GetHitShape() const;

    void CheckFlightHits(float DeltaTime);
    void HandleCheckedHit(AActor* TargetActor);

    void EndFlight();

    FTimerHandle ActivationTimerHandle;

    bool bActivated = false;
    bool bHandlingHits = false;

    bool bChecksCharactersOnRecords = false;
    int32 HitCheckHandle = 0;
    double LastCheckedTime = 0.0;
    FVector LastCheckedLocation = FVector::ZeroVector;

    bool bFlightOver = false;
    double FlightOverTime = 0.0;

    float StartTime;

    FVector StartLocation;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> HandledActors;
};
