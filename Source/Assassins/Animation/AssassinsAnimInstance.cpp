// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AssassinsAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAssassinsAnimInstance::UAssassinsAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GroundSpeed = 0.0f;
}

void UAssassinsAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (ACharacter* OwningCharacter = Cast<ACharacter>(TryGetPawnOwner()))
	{
		GroundSpeed = OwningCharacter->GetVelocity().Size2D();
	}
}
