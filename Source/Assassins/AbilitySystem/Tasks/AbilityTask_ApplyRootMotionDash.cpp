// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Character/Movements/AssassinsRootMotionSource.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_DASHING, "Status.Dashing");

UAbilityTask_ApplyRootMotionDash::UAbilityTask_ApplyRootMotionDash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbilityTask_ApplyRootMotionDash::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
}

void UAbilityTask_ApplyRootMotionDash::OnDestroy(bool AbilityIsEnding)
{
	RestoreMovementAndCollision();

	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_STATUS_DASHING, 0);
	}

	Super::OnDestroy(AbilityIsEnding);
}

void UAbilityTask_ApplyRootMotionDash::AbortMoveAndDash()
{
	if (APawn* MyPawn = Cast<APawn>(GetAvatarActor()))
	{
		if (AController* MyPawnController = MyPawn->GetController())
		{
			MyPawnController->StopMovement();
		}
	}

	for (auto It = AbilitySystemComponent->GetKnownTaskIterator(); It; ++It)
	{
		auto KnownTask = It->Get();
		if (KnownTask != this && KnownTask->IsA<UAbilityTask_ApplyRootMotionDash>())
		{
			KnownTask->EndTask();
		}
	}
}

void UAbilityTask_ApplyRootMotionDash::SetMovementAndCollision()
{
	PreviousMovementMode = MovementComponent->MovementMode;
	PreviousCustomMode = MovementComponent->CustomMovementMode;
	MovementComponent->SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_Dashing);

	if (ACharacter* MyCharacter = Cast<ACharacter>(AbilitySystemComponent->GetAvatarActor()))
	{
		StartLocation = MyCharacter->GetActorLocation();

		if (UCapsuleComponent* CapsuleComp = MyCharacter->GetCapsuleComponent())
		{
			CapsuleComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
			CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
	}
}

void UAbilityTask_ApplyRootMotionDash::RestoreMovementAndCollision()
{
	if (MovementComponent.IsValid())
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
		MovementComponent->SetMovementMode(PreviousMovementMode, PreviousCustomMode);
	}

	if (AbilitySystemComponent.IsValid())
	{
		if (ACharacter* MyCharacter = Cast<ACharacter>(AbilitySystemComponent->GetAvatarActor()))
		{
			if (UCapsuleComponent* CapsuleComp = MyCharacter->GetCapsuleComponent())
			{
				CapsuleComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
				CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			}
		}
	}
}

void UAbilityTask_ApplyRootMotionDash::CheckDashFinish()
{
	AActor* MyActor = GetAvatarActor();

	const float AcceptRadiusSqr = AcceptRadius * AcceptRadius;
	const bool bReachedDestination = FVector::DistSquared(TargetLocation, MyActor->GetActorLocation()) <= AcceptRadiusSqr;

	if (bReachedDestination)
	{
		// Task has finished
		bIsFinished = true;
		MyActor->SetActorLocation(TargetLocation);

		if (!bIsSimulating)
		{
			if (ShouldBroadcastAbilityTaskDelegates())
			{
				OnFinished.Broadcast();
			}
			EndTask();
		}
	}
}

///////////////////////////
// UAbilityTask_DashTo
///////////////////////////

UAbilityTask_DashTo::UAbilityTask_DashTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AcceptRadius = 50.0f;
}

UAbilityTask_DashTo* UAbilityTask_DashTo::DashTo(UGameplayAbility* OwningAbility, FName TaskInstanceName, FVector TargetLocation, float DashSpeed, float AcceptRadius, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish)
{
	UAbilityTask_DashTo* MyTask = NewAbilityTask<UAbilityTask_DashTo>(OwningAbility, TaskInstanceName);

	MyTask->ForceName = TaskInstanceName;
	MyTask->TargetLocation = TargetLocation;
	MyTask->DashSpeed = DashSpeed;
	MyTask->AcceptRadius = FMath::Max(MyTask->AcceptRadius, AcceptRadius);
	MyTask->FinishVelocityMode = VelocityOnFinishMode;
	MyTask->FinishSetVelocity = SetVelocityOnFinish;
	MyTask->FinishClampVelocity = ClampVelocityOnFinish;

	MyTask->SharedInitAndApply();

	return MyTask;
}

void UAbilityTask_DashTo::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	if (GetAvatarActor())
	{
		CheckDashFinish();
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UAbilityTask_DashTo::SharedInitAndApply()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	check(ASC);

	AbortMoveAndDash();

	if (ASC->AbilityActorInfo->MovementComponent.Get())
	{
		MovementComponent = Cast<UAssassinsCharacterMovementComponent>(ASC->AbilityActorInfo->MovementComponent.Get());
		if (MovementComponent.IsValid())
		{
			SetMovementAndCollision();

			ASC->SetLooseGameplayTagCount(TAG_STATUS_DASHING, 1);

			ForceName = ForceName.IsNone() ? FName("AbilityTaskDashTo") : ForceName;
			TSharedPtr<FRootMotionSource_MoveToConstantSpeed> MoveToForce = MakeShared<FRootMotionSource_MoveToConstantSpeed>();
			MoveToForce->InstanceName = ForceName;
			MoveToForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			MoveToForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
			MoveToForce->Priority = 900;
			MoveToForce->TargetLocation = TargetLocation;
			MoveToForce->StartLocation = StartLocation;
			MoveToForce->Speed = DashSpeed;
			MoveToForce->FinishVelocityParams.Mode = FinishVelocityMode;
			MoveToForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
			MoveToForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
			RootMotionSourceID = MovementComponent->ApplyRootMotionSource(MoveToForce);
		}
	}
}

//////////////////////////////
// UAbilityTask_DashToActor
//////////////////////////////

UAbilityTask_DashToActor::UAbilityTask_DashToActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AcceptRadius = 80.0f;
}

UAbilityTask_DashToActor* UAbilityTask_DashToActor::DashToActor(UGameplayAbility* OwningAbility, FName TaskInstanceName, AActor* TargetActor, float DashSpeed, float AcceptRadius, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish)
{
	UAbilityTask_DashToActor* MyTask = NewAbilityTask<UAbilityTask_DashToActor>(OwningAbility, TaskInstanceName);

	MyTask->ForceName = TaskInstanceName;
	MyTask->TargetActor = TargetActor;
	MyTask->DashSpeed = DashSpeed;
	MyTask->AcceptRadius = FMath::Max(MyTask->AcceptRadius, AcceptRadius);
	MyTask->FinishVelocityMode = VelocityOnFinishMode;
	MyTask->FinishSetVelocity = SetVelocityOnFinish;
	MyTask->FinishClampVelocity = ClampVelocityOnFinish;

	MyTask->SharedInitAndApply();

	return MyTask;
}

void UAbilityTask_DashToActor::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	AActor* MyActor = GetAvatarActor();
	if (MyActor)
	{
		// Update target location
		{
			const FVector PreviousTargetLocation = TargetLocation;
			if (UpdateTargetLocation(DeltaTime))
			{
				SetRootMotionTargetLocation(TargetLocation);
			}
			else
			{
				// TargetLocation not updated - TargetActor not around anymore, continue on to last set TargetLocation
			}
		}

		CheckDashFinish();
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UAbilityTask_DashToActor::SharedInitAndApply()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	check(ASC);

	AbortMoveAndDash();

	if (ASC->AbilityActorInfo->MovementComponent.Get())
	{
		MovementComponent = Cast<UAssassinsCharacterMovementComponent>(ASC->AbilityActorInfo->MovementComponent.Get());
		if (MovementComponent.IsValid())
		{
			AActor* MyActor = ASC->GetAvatarActor();
			if (TargetActor && MyActor)
			{
				StartLocation = MyActor->GetActorLocation();

				const FVector ToTarget = (TargetActor->GetActorLocation() - MyActor->GetActorLocation()).GetSafeNormal2D();
				TargetLocation = TargetActor->GetActorLocation() - AcceptRadius * ToTarget;
			}

			SetMovementAndCollision();

			ASC->SetLooseGameplayTagCount(TAG_STATUS_DASHING, 1);

			ForceName = ForceName.IsNone() ? FName("AbilityTaskDashToActor") : ForceName;
			TSharedPtr<FRootMotionSource_MoveToDynamicConstantSpeed> MoveToActorForce = MakeShared<FRootMotionSource_MoveToDynamicConstantSpeed>();
			MoveToActorForce->InstanceName = ForceName;
			MoveToActorForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			MoveToActorForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
			MoveToActorForce->Priority = 900;
			MoveToActorForce->InitialTargetLocation = TargetLocation;
			MoveToActorForce->TargetLocation = TargetLocation;
			MoveToActorForce->StartLocation = StartLocation;
			MoveToActorForce->Speed = DashSpeed;
			MoveToActorForce->FinishVelocityParams.Mode = FinishVelocityMode;
			MoveToActorForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
			MoveToActorForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
			RootMotionSourceID = MovementComponent->ApplyRootMotionSource(MoveToActorForce);
		}
	}
}

bool UAbilityTask_DashToActor::UpdateTargetLocation(float DeltaTime)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		AActor* MyActor = ASC->GetAvatarActor();
		if (TargetActor && MyActor)
		{
			const FVector ToTarget = (TargetActor->GetActorLocation() - MyActor->GetActorLocation()).GetSafeNormal2D();
			TargetLocation = TargetActor->GetActorLocation() - AcceptRadius * ToTarget;
			return true;
		}
	}
	return false;
}

void UAbilityTask_DashToActor::SetRootMotionTargetLocation(FVector NewTargetLocation)
{
	if (MovementComponent.IsValid())
	{
		TSharedPtr<FRootMotionSource> RMS = MovementComponent->GetRootMotionSourceByID(RootMotionSourceID);
		if (RMS.IsValid())
		{
			if (RMS->GetScriptStruct() == FRootMotionSource_MoveToDynamicForce::StaticStruct())
			{
				FRootMotionSource_MoveToDynamicForce* MoveToActorForce = static_cast<FRootMotionSource_MoveToDynamicForce*>(RMS.Get());
				if (MoveToActorForce)
				{
					MoveToActorForce->SetTargetLocation(TargetLocation);
				}
			}
		}
	}
}