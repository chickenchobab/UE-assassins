// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "AITypes.h"

#include "AssassinsTargetChasingComponent.generated.h"

namespace EPathFollowingResult { enum Type : int; }

DECLARE_DELEGATE_TwoParams(FChaseTargetDelegate, AActor*, float);
DECLARE_DELEGATE(FStopChaseDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChaseCompletedSignature, FAIRequestID, RequestID, EPathFollowingResult::Type, Result);

/**
* Controller component for targeted ability which makes
* the agent to chase the target and do something on arriving.
*/
UCLASS(BlueprintType, NotBlueprintable)
class ASSASSINS_API UAssassinsTargetChasingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAssassinsTargetChasingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Gameplay")
	void SetKeepChase(bool bKeep) { bKeepChase = bKeep; }
	UFUNCTION(BlueprintCallable, Category = "Assassins|Gameplay")
	bool ShouldKeepChase() const { return bKeepChase; }
	
	UFUNCTION(BlueprintCallable, Category = "Assassins|Gameplay")
	void ChaseTarget(AActor* Target, float AcceptRadius);
	UFUNCTION(BlueprintCallable, Category = "Assassins|Gameplay")
	void StopChase();

	void SetTargetState(AActor* Target, float AcceptRadius);
	UFUNCTION(BlueprintCallable, Category = "Assassins|Gameplay")
	void ResetTargetState();

public:

	FChaseTargetDelegate ChaseTargetDelegate;
	FStopChaseDelegate StopChaseDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Assassins|Gameplay")
	FChaseCompletedSignature HandleChaseCompleted;


	UPROPERTY()
	TWeakObjectPtr<AActor> CachedTarget;

	float CachedAcceptRadius;

private:

	// Set by someone outside the component
	bool bKeepChase;
	// Give it a one tick chance to receive move complete.
	bool bKeepChaseBefore;
};
