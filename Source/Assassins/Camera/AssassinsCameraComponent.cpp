// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/AssassinsCameraComponent.h"
#include "Camera/AssassinsCameraMode.h"

UAssassinsCameraComponent::UAssassinsCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAssassinsCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	UpdateCameraMode();

	check(CurrentCameraMode);

	FAssassinsCameraModeView CameraModeView;
	CurrentCameraMode->EvaluateCameraMode(DeltaTime, CameraModeView);

	// Keep player controller in sync with the latest view.
	if (APawn* TargetPawn = Cast<APawn>(GetTargetActor()))
	{
		if (APlayerController* PC = TargetPawn->GetController<APlayerController>())
		{
			PC->SetControlRotation(CameraModeView.ControlRotation);
		}
	}

	// Apply any offset that was added to the field of view.
	CameraModeView.FieldOfView += FieldOfViewOffset;
	FieldOfViewOffset = 0.0f;

	// Keep camera component in sync with the latest view.
	SetWorldLocationAndRotation(CameraModeView.Location, CameraModeView.Rotation);
	FieldOfView = CameraModeView.FieldOfView;

	// Fill in desired view.
	DesiredView.Location = CameraModeView.Location;
	DesiredView.Rotation = CameraModeView.Rotation;
	DesiredView.FOV = CameraModeView.FieldOfView;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;

	// See if the CameraActor wants to override the PostProcess settings used.
	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		DesiredView.PostProcessSettings = PostProcessSettings;
	}

	if (IsXRHeadTrackedCamera())
	{
		// In XR much of the camera behavior above is irrelevant, but the post process settings are not.
		Super::GetCameraView(DeltaTime, DesiredView);
	}
}

void UAssassinsCameraComponent::UpdateCameraMode()
{
	if (DetermineCameraModeDelegate.IsBound())
	{
		if (const TSubclassOf<UAssassinsCameraMode> CameraMode = DetermineCameraModeDelegate.Execute())
		{
			SetCurrentCameraMode(CameraMode);
		}
	}
}

void UAssassinsCameraComponent::SetCurrentCameraMode(TSubclassOf<UAssassinsCameraMode> CameraModeClass)
{
	check(CameraModeClass);

	// First see if we already created one.
	if ((CurrentCameraMode != nullptr) && (CurrentCameraMode->GetClass() == CameraModeClass))
	{
		return;
	}

	// Not found, so we need to create it.
	if ((CurrentCameraMode != nullptr) && (CurrentCameraMode->GetClass() != CameraModeClass))
	{
		CurrentCameraMode->OnDiselected();
	}

	UAssassinsCameraMode* NewCameraMode = NewObject<UAssassinsCameraMode>(this, CameraModeClass, NAME_None, RF_NoFlags);
	check(NewCameraMode);

	CurrentCameraMode = NewCameraMode;
	CurrentCameraMode->OnSelected();
}