// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/AssassinsCameraMode_TopDown.h"

void UAssassinsCameraMode_TopDown::UpdateView(float DeltaTime)
{
	FVector PivotLocation = GetPivotLocation() - DefaultPivotRotation.Vector() * CameraLoftDistance;

	FRotator PivotRotation = DefaultPivotRotation;

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}
