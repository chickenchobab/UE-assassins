// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/AssassinsCameraMode.h"
#include "AssassinsCameraMode_TopDown.generated.h"

enum class ECardinalDirection : uint8;

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class UAssassinsCameraMode_TopDown : public UAssassinsCameraMode
{
	GENERATED_BODY()

public:

	UAssassinsCameraMode_TopDown();
	
protected:

	//~UAssassinsCameraMode interface
	virtual void UpdateView(float DeltaTime) override;
	virtual void OnSelected() override;
	virtual void OnDiselected() override;
	//~End of UAssassinsCameraMode interface

	void MovePivotLoaction(ECardinalDirection CardinalDirection);
	void BeginFocusOnTargetActor();
	void EndFocusOnTargetActor();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "TopDown")
	FRotator DefaultPivotRotation;

	UPROPERTY(EditDefaultsOnly, Category = "TopDown")
	double CameraLoftDistance;

	UPROPERTY(EditDefaultsOnly, Category = "TopDown")
	float PivotMoveSpeed;

private:

	FVector CurrentPivotLocation;

	bool bIsFocusingOnTargetActor;
};
