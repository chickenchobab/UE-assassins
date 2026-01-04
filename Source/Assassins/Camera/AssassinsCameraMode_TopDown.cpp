// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/AssassinsCameraMode_TopDown.h"
#include "UI/AssassinsGameViewportClient.h"
#include "Components/InputComponent.h"

UAssassinsCameraMode_TopDown::UAssassinsCameraMode_TopDown()
{
	bIsFocusingOnTargetActor = false;
}

void UAssassinsCameraMode_TopDown::UpdateView(float DeltaSeconds)
{
	if (bIsFocusingOnTargetActor)
	{
		CurrentPivotLocation = GetPivotLocation() - DefaultPivotRotation.Vector() * CameraLoftDistance;
	}

	FVector PivotLocation = CurrentPivotLocation;
	FRotator PivotRotation = DefaultPivotRotation;

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}

void UAssassinsCameraMode_TopDown::OnSelected()
{
	CurrentPivotLocation = GetPivotLocation() - DefaultPivotRotation.Vector() * CameraLoftDistance;

	// Me: Whenever the cursor reaches the edges of the viewport, move camera focus.
	if (UWorld* World = GetWorld())
	{
		if (UAssassinsGameViewportClient* ViewportClient = Cast<UAssassinsGameViewportClient>(World->GetGameViewport()))
		{
			ViewportClient->OnCursorAtViewportEdge.AddUObject(this, &ThisClass::MovePivotLoaction);
			ViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
		}
	}

	// Me: Bind space bar to RestorePivotLocation
	if (AActor* TargetActor = GetTargetActor())
	{
		if (APawn* Pawn = Cast<APawn>(TargetActor))
		{
			if (Pawn->InputComponent)
			{
				Pawn->InputComponent->BindKey(FKey(TEXT("SpaceBar")), EInputEvent::IE_Pressed, this, &ThisClass::BeginFocusOnTargetActor);
				Pawn->InputComponent->BindKey(FKey(TEXT("SpaceBar")), EInputEvent::IE_Released, this, &ThisClass::EndFocusOnTargetActor);
			}
		}
	}
}

void UAssassinsCameraMode_TopDown::OnDiselected()
{
	if (UWorld* World = GetWorld())
	{
		if (UAssassinsGameViewportClient* ViewportClient = Cast<UAssassinsGameViewportClient>(World->GetGameViewport()))
		{
			ViewportClient->OnCursorAtViewportEdge.RemoveAll(this);
			ViewportClient->SetMouseLockMode(EMouseLockMode::DoNotLock);
		}
	}

	// Me: Unbind space bar
	if (AActor* TargetActor = GetTargetActor())
	{
		if (APawn* Pawn = Cast<APawn>(TargetActor))
		{
			if (Pawn->InputComponent)
			{
				Pawn->InputComponent->KeyBindings.RemoveAll([this](FInputKeyBinding KeyBinding) 
				{ 
					return KeyBinding.KeyDelegate.GetUObject() == this;
				});
			}
		}
	}
}

void UAssassinsCameraMode_TopDown::MovePivotLoaction(ECardinalDirection CardinalDirection)
{
	if (bIsFocusingOnTargetActor)
	{
		return;
	}

	float DeltaSeconds = 0.0f;
	if (UWorld* World = GetWorld())
	{
		DeltaSeconds = World->GetDeltaSeconds();
	}

	// Me: Transformation expensive?
	FVector DeltaLocation = FVector::VectorPlaneProject(View.Rotation.RotateVector(FVector::RightVector), FVector::UpVector);
	if (CardinalDirection == ECardinalDirection::Up)
	{
		DeltaLocation = FVector::VectorPlaneProject(View.Rotation.RotateVector(FVector::UpVector), FVector::UpVector);
	}
	else if (CardinalDirection == ECardinalDirection::Down)
	{
		DeltaLocation = FVector::VectorPlaneProject(View.Rotation.RotateVector(-FVector::UpVector), FVector::UpVector);
	}
	else if (CardinalDirection == ECardinalDirection::Left)
	{
		DeltaLocation = FVector::VectorPlaneProject(View.Rotation.RotateVector(-FVector::RightVector), FVector::UpVector);
	}
	
	if (!DeltaLocation.IsNearlyZero())
	{
		DeltaLocation.Normalize();
	}

	const FVector CurrentLocation = CurrentPivotLocation;
	const FVector NewLocation = CurrentLocation + PivotMoveSpeed * DeltaLocation;

	CurrentPivotLocation = FMath::VInterpConstantTo(CurrentLocation, NewLocation, DeltaSeconds, PivotMoveSpeed);
}

void UAssassinsCameraMode_TopDown::BeginFocusOnTargetActor()
{
	bIsFocusingOnTargetActor = true;

	CurrentPivotLocation = GetPivotLocation() - DefaultPivotRotation.Vector() * CameraLoftDistance;
}

void UAssassinsCameraMode_TopDown::EndFocusOnTargetActor()
{
	bIsFocusingOnTargetActor = false;
}
