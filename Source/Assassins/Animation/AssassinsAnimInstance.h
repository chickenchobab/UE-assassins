// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Animation/AnimInstance.h"
#include "AssassinsAnimInstance.generated.h"

/**
 * The base game animation instance class used by this project
 */
UCLASS()
class UAssassinsAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	UAssassinsAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UAnimInstance interface
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	//~End of UAnimInstance interface

protected:

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Movement")
	float GroundSpeed;
};
