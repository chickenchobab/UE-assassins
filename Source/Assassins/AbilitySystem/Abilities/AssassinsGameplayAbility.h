// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "Engine/HitResult.h"
#include "AssassinsGameplayAbility.generated.h"

class UAssassinsAbilitySystemComponent;
class AAssassinsPlayerController;
class AAssassinsCharacter;
class AAssassinsProjectile;
class UAssassinsAnimInstance;
class UAnimMontage;
struct FCollisionObjectQueryParams;

DECLARE_DYNAMIC_DELEGATE(FAbilityReplicatedDelegate);
DECLARE_DYNAMIC_DELEGATE(FInPredictionWindowDelegate);

UENUM(BlueprintType)
enum class EAbilityCustomReplicatedEvent : uint8
{
	GameCustom1,
	GameCustom2,
	GameCustom3,
	GameCustom4,
	GameCustom5,
	GameCustom6
};

/**
 * EAssassinsAbilityActivationPolicy
 * 
 * Defines how an ability is meant to activate.
 */
UENUM(BlueprintType)
enum class EAssassinsAbilityActivationPolicy : uint8
{
	// Try to activate the ability when the input is triggered(It can also be activated by gameplay event).
	OnInputTriggered,

	// Continually try to activate the ability while the input is active.
	WhileInputActive,

	// Try to activate the ability when an avatar is assigned.
	OnSpawn
};

/**
 * UAssassinsMontageWithTiming
 *
 * A montage paired with the moment its effect happens, e.g. the hit of an attack.
 * The MontageData_* assets are instances of it, through the DA_MontageWithTiming blueprint.
 */
UCLASS(BlueprintType)
class ASSASSINS_API UAssassinsMontageWithTiming : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> Montage;

	// Double, to match the blueprint variable it replaces.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assassins|Ability")
	double Timing = 0.0;
};

/**
 *
 */
UCLASS(Abstract, HideCategories = Input, Meta = (ShourtTooltip = "The base gameplay ability class used by this project."))
class ASSASSINS_API UAssassinsGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAssassinsGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Assassins|Ability")
	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	AAssassinsPlayerController* GetAssassinsPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	AController* GetControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	AAssassinsCharacter* GetAssassinsCharacterFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	UAssassinsAnimInstance* GetAssassinsAnimInstanceFromActorInfo() const;

	EAssassinsAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	const FGameplayTagContainer& GetCancelledByTags() const { return CancelledByTags; }

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	UAssassinsTargetChasingComponent* GetTargetChasingComponentFromController() const;

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	AActor* GetCurrentCursorTarget() const;

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	// The engine event a custom one stands for: GameCustom1 and on.
	static EAbilityGenericReplicatedEvent::Type ToGenericReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent);

protected:

    //~UGameplayAbility interface
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
    virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    //~End of UGameplayAbility interface

    UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
    FGameplayEffectSpecHandle MakeEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass);

    UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
    FActiveGameplayEffectHandle ApplyGameplayEffectSpecToTargetActor(const FGameplayEffectSpecHandle& SpecHandle, AActor* TargetActor);

	// A spec of the effect, applied to the target. Nothing when the target has no ability system, e.g. once it is gone.
	FActiveGameplayEffectHandle ApplyEffectToTarget(TSubclassOf<UGameplayEffect> EffectClass, AActor* TargetActor);

	// Spawns a projectile of the ability. It belongs to ProjectileOwner(the avatar unless given, e.g. Zed's shadow that
	// throws it), and the avatar is its instigator: what it hits knows who threw it(AAssassinsProjectile::IsValidTarget).
	// Null when there is no avatar or no class.
	AAssassinsProjectile* SpawnAbilityProjectile(TSubclassOf<AAssassinsProjectile> ProjectileClass, const FTransform& SpawnTransform, AActor* ProjectileOwner = nullptr) const;

	// The enemies(IsValidEnemy) a sphere of Radius finds as it rises from the ground below Center up to TopZ, with the first
	// hit on each: every enemy is there once, whichever of its components the sweep finds. The avatar and SourceActor are
	// left out.
	TArray<FHitResult> SweepForEnemies(const FVector& Center, double Radius, double TopZ, const FCollisionObjectQueryParams& ObjectQueryParams, const AActor* SourceActor = nullptr) const;

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability", meta = (DataTablePin = "CurveTable"))
	float EvaluateCurveTableRowByAbilityLevel(UCurveTable* CurveTable, FName RowName, const FString& ContextString) const;
	
	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	bool IsValidEnemy(AActor* TargetActor) const;

	// Me: Sets or clears an ability-scoped status tag on the avatar actor.
	// Gameplay effect should be used to add a tag to another actor
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void AddTagToAvatar(FGameplayTag Tag);
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void RemoveTagFromAvatar(FGameplayTag Tag);

	// Me: Dynamically makes the ability cancellable by the other abilities, inputs or status.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void AddCancelledByTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void SetAvatarLocationAndRotation(const FVector& GoalLocation, const FRotator& GoalRotation);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void ServerSetReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent);
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void ClientSetReplicatedEvent(EAbilityCustomReplicatedEvent CustomEvent);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability|RPC")
	void CallOrAddReplicatedDelegate(EAbilityCustomReplicatedEvent CustomEvent, FAbilityReplicatedDelegate ReplicatedDelegate, bool bUnbindCalledDelegate = true);

	// For what the owning client starts and the server follows. LocalAction runs inside a new prediction window,
	// and the server's handler of the event(UAbilityTask_WaitReplicatedEvent) runs inside a window of the same key,
	// so what both sides apply is matched up. Unlike WaitNetSync, every purpose gets its own event.
	void SendPredictedEventToServer(EAbilityCustomReplicatedEvent CustomEvent, TFunctionRef<void()> LocalAction);

protected:
	
	// Defines how this ability is meant to activate.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	EAssassinsAbilityActivationPolicy ActivationPolicy;

	// Me: Structures(the team base for instance) are only meant to be hit by basic attacks,
	// so an ability has to opt in before IsValidEnemy accepts one as a target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	bool bCanTargetStructure;

private:

	UPROPERTY()
	FGameplayTagContainer CancelledByTags;

	UPROPERTY()
	FGameplayTagContainer AvatarStatusTags;
};
