// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/AssassinsCameraMode.h"
#include "Camera/AssassinsCameraComponent.h"


//////////////////////////////////////////////////////////////////////////
// FAssassinsCameraModeView
//////////////////////////////////////////////////////////////////////////
FAssassinsCameraModeView::FAssassinsCameraModeView()
	: Location(ForceInit)
	, Rotation(ForceInit)
	, ControlRotation(ForceInit)
	, FieldOfView(80.0f)
{
}


//////////////////////////////////////////////////////////////////////////
// UAssassinsCameraMode
//////////////////////////////////////////////////////////////////////////
UAssassinsCameraMode::UAssassinsCameraMode()
{
	FieldOfView = 80.0f;
	ViewPitchMin = -89.9f;
	ViewPitchMax = 89.9f;
}

UAssassinsCameraComponent* UAssassinsCameraMode::GetAssassinsCameraComponent() const
{
	return CastChecked<UAssassinsCameraComponent>(GetOuter());
}

AActor* UAssassinsCameraMode::GetTargetActor() const
{
	const UAssassinsCameraComponent* AssassinsCameraComponent = GetAssassinsCameraComponent();

	return AssassinsCameraComponent->GetTargetActor();
}

FVector UAssassinsCameraMode::GetPivotLocation() const
{
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		return TargetPawn->GetPawnViewLocation();
	}

	return TargetActor->GetActorLocation();
}

FRotator UAssassinsCameraMode::GetPivotRotation() const
{
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		return TargetPawn->GetViewRotation();
	}

	return TargetActor->GetActorRotation();
}

void UAssassinsCameraMode::EvaluateCameraMode(float DeltaTime, FAssassinsCameraModeView& OutCameraModeView)
{
	UpdateView(DeltaTime);
	OutCameraModeView = View;
}

void UAssassinsCameraMode::UpdateView(float DeltaTime)
{
	FVector PivotLocation = GetPivotLocation();
	FRotator PivotRotation = GetPivotRotation();

	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}
