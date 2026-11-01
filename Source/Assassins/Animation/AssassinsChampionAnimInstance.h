#pragma once

#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimNodeReference.h"
#include "Animation/AssassinsAnimInstance.h"
#include "AssassinsChampionAnimInstance.generated.h"

class UAnimMontage;
class UAssassinsAnimProfile;

UCLASS(Abstract)
class UAssassinsChampionAnimInstance : public UAssassinsAnimInstance
{
	GENERATED_BODY()

public:

	UAssassinsChampionAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC) override;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void UpdateLocomotionSM(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupIdleState(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void UpdateIdleState(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupIdleAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupIdleOverrideAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupWalkAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupWalkOverrideAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupRunAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupRunOverrideAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupStartAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void SetupIdleBreakAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void ResetStartAndStopAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void HandlePossibleMontageToStop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void HandlePossibleMontageToCycle(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintPure, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	bool CanPlayIdleBreaks() const;

	UFUNCTION(BlueprintPure, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	bool ShouldOverrideIdleSequence() const;

	UFUNCTION(BlueprintPure, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	bool ShouldOverrideCycleSequence() const;

protected:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

private:

	void UpdateLocationData(double DeltaTime);
	void UpdateVelocityData();
	void UpdateAccelerationData();
	void UpdateMontageData();

	void ChooseIdleBreakDelayTime();
	void ProcessIdleBreakTransition(double DeltaTime);
	void ResetIdleBreakTransition();

	UFUNCTION()
	void HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

public:

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Assassins|Animation")
	TObjectPtr<UAssassinsAnimProfile> AnimProfile;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	bool HasStartSequence = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	bool HasStopSequence = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	double WalkToRunSpeed = 500.0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	double DisplacementSinceLastUpdate = 0.0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	double DisplacementSpeed = 0.0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	FVector WorldVelocity = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	bool HasVelocity = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	FVector WorldAcceleration = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	bool HasAcceleration = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Locomotion")
	bool IsFirstUpdate = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Tags")
	bool GameplayTag_HasActivatedAbility = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Idle Breaks")
	double IdleBreakDelayTime = 0.0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Idle Breaks")
	double TimeUntilNextIdleBreak = 0.0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Montage")
	bool IsMontagePlaying = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Montage")
	bool IsMontageBlendingOut = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Assassins|Animation|Montage")
	TObjectPtr<UAnimMontage> BlendingOutMontage;

private:

	FVector OwnerLocation = FVector::ZeroVector;
	FVector OwnerVelocity = FVector::ZeroVector;
	FVector OwnerAcceleration = FVector::ZeroVector;
	bool bAnyMontagePlayingThisFrame = false;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
};
