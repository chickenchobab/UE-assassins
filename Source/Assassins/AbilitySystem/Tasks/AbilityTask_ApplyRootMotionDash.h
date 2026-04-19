// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask_ApplyRootMotion_Base.h"
#include "AbilityTask_ApplyRootMotionDash.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAssassinsDashDelegate);

UCLASS()
class ASSASSINS_API UAbilityTask_ApplyRootMotionDash : public UAbilityTask_ApplyRootMotion_Base
{
	GENERATED_BODY()

public:

	UAbilityTask_ApplyRootMotionDash(const FObjectInitializer& ObjectInitializer);

	//UGameplayTask interface
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool AbilityIsEnding) override;
	//~End of UGameplayTask interface

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	FVector GetCurrentTargetLocation() const { return TargetLocation; }

	UPROPERTY(BlueprintAssignable)
	FAssassinsDashDelegate OnTickTask;

	UPROPERTY(BlueprintAssignable)
	FAssassinsDashDelegate OnCancelled;
	UPROPERTY(BlueprintAssignable)
	FAssassinsDashDelegate OnFinished;

protected:

	// Me: Abort existing move
	// and dash(es) because regarding gameplay abilities have allowed it
	void AbortMoveAndDash();

	void SetMovementMode();
	void ResetMovementMode();

	void CheckDashFinish();

protected:

	UPROPERTY(Replicated)
	FVector StartLocation;

	UPROPERTY(Replicated)
	FVector TargetLocation;

	UPROPERTY(Replicated)
	float DashSpeed;

	UPROPERTY(Replicated)
	float AcceptRadius;

	TEnumAsByte<EMovementMode> PreviousMovementMode;
	uint8 PreviousCustomMode;
};

UCLASS()
class ASSASSINS_API UAbilityTask_DashTo : public UAbilityTask_ApplyRootMotionDash
{
	GENERATED_BODY()

public:

	UAbilityTask_DashTo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_DashTo* DashTo(UGameplayAbility* OwningAbility, FName TaskInstanceName, FVector InTargetLocation, float InDashSpeed, float InAcceptRadius, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish);

	//~UGameplayTask interface
	virtual void TickTask(float DeltaTime) override;
	//~End of UGameplayTask interface

protected:

	virtual void SharedInitAndApply() override;

	void AdjustTargetLocation();
};

UCLASS()
class ASSASSINS_API UAbilityTask_DashToActor : public UAbilityTask_ApplyRootMotionDash
{
	GENERATED_BODY()

public:

	UAbilityTask_DashToActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_DashToActor* DashToActor(UGameplayAbility* OwningAbility, FName TaskInstanceName, AActor* TargetActor, float InDashSpeed, float InAcceptRadius, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish);

	//~UGameplayTask interface
	virtual void TickTask(float DeltaTime) override;
	//~End of UGameplayTask interface

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

protected:

	virtual void SharedInitAndApply() override;

	bool UpdateTargetLocation(float DeltaTime);

	void SetRootMotionTargetLocation(FVector NewTargetLocation);

protected:

	UPROPERTY(Replicated)
	TObjectPtr<AActor> TargetActor;
};
