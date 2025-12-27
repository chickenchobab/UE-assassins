// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/AssassinsCameraMode.h"
#include "AssassinsCameraMode_TopDown.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class UAssassinsCameraMode_TopDown : public UAssassinsCameraMode
{
	GENERATED_BODY()
	
protected:

	//~UAssassinsGameMode interface
	virtual void UpdateView(float DeltaTime) override;
	//~End of UAssassinsGameMode interface

protected:

	UPROPERTY(EditDefaultsOnly, Category = "TopDown")
	FRotator DefaultPivotRotation;

	UPROPERTY(EditDefaultsOnly, Category = "TopDown")
	double CameraLoftDistance;
};
