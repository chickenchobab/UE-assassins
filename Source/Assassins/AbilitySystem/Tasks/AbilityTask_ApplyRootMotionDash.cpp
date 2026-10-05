// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Character/Movements/AssassinsRootMotionSource.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DASHING, "Status.Dashing");

UAbilityTask_ApplyRootMotionDash::UAbilityTask_ApplyRootMotionDash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbilityTask_ApplyRootMotionDash::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	OnTickTask.Broadcast();
}

void UAbilityTask_ApplyRootMotionDash::OnDestroy(bool AbilityIsEnding)
{
	if (!bIsFinished && ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast();
		UE_LOG(LogTemp, Display, TEXT("Dash has been cancelled"));
	}

	ResetMovementMode();

	// Mostly gone already: the movement takes it off on the move that ends the dash. Not when the dash was cut short.
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_DASHING, 0);
	}

	Super::OnDestroy(AbilityIsEnding);
}

void UAbilityTask_ApplyRootMotionDash::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityTask_ApplyRootMotionDash, StartLocation);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionDash, TargetLocation);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionDash, DashSpeed);
	DOREPLIFETIME(UAbilityTask_ApplyRootMotionDash, AcceptRadius);
}

void UAbilityTask_ApplyRootMotionDash::PreDestroyFromReplication()
{
	bIsFinished = true;
	EndTask();
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
		UAbilityTask_ApplyRootMotionDash* OtherDash = Cast<UAbilityTask_ApplyRootMotionDash>(It->Get());
		if ((OtherDash == nullptr) || (OtherDash == this))
		{
			continue;
		}

		// A dash that reached its target ends as finished, as it would on its next tick: the move that ends a dash lets
		// the next one start before that tick(UAssassinsCharacterMovementComponent::EndDashingStatus). Only a dash still
		// on its way is cut short.
		OtherDash->CheckDashFinish();
		if (!OtherDash->bIsFinished)
		{
			OtherDash->EndTask();
		}
	}
}

void UAbilityTask_ApplyRootMotionDash::SetMovementMode()
{
	PreviousMovementMode = MovementComponent->MovementMode;
	PreviousCustomMode = MovementComponent->CustomMovementMode;
	MovementComponent->SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_Dashing);
}

void UAbilityTask_ApplyRootMotionDash::ResetMovementMode()
{
	if (MovementComponent.IsValid())
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
		MovementComponent->SetMovementMode(PreviousMovementMode, PreviousCustomMode);
	}
}

void UAbilityTask_ApplyRootMotionDash::CheckDashFinish()
{
	// The root motion source ends the dash, on the move that reaches the target(FRootMotionSource_MoveToDynamicConstantSpeed),
	// and the movement removes it on the next. Only the ability hears it here: where the dash ends and when the movement
	// leaves the dash mode are up to the moves, the same on the owning client and the server however their frames go.
	const TSharedPtr<FRootMotionSource> RootMotionSource = MovementComponent.IsValid() ? MovementComponent->GetRootMotionSourceByID(RootMotionSourceID) : nullptr;
	if (RootMotionSource.IsValid() && !RootMotionSource->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		return;
	}

	// Task has finished
	bIsFinished = true;

	if (!bIsSimulating)
	{
		GetAvatarActor()->ForceNetUpdate();
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnFinished.Broadcast();
		}
		EndTask();
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
			AdjustTargetLocation();
			
			SetMovementMode();
			// Set capsule collision
			ASC->SetLooseGameplayTagCount(TAG_DASHING, 1);

			ForceName = ForceName.IsNone() ? FName("AbilityTaskDashTo") : ForceName;
			TSharedPtr<FRootMotionSource_MoveToDynamicConstantSpeed> MoveToForce = MakeShared<FRootMotionSource_MoveToDynamicConstantSpeed>();
			MoveToForce->InstanceName = ForceName;
			MoveToForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			MoveToForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
			MoveToForce->Priority = 900;
			MoveToForce->TargetLocation = TargetLocation;
			MoveToForce->StartLocation = StartLocation;
			MoveToForce->Speed = DashSpeed;
			MoveToForce->AcceptRadius = AcceptRadius;
			MoveToForce->FinishVelocityParams.Mode = FinishVelocityMode;
			MoveToForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
			MoveToForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
			RootMotionSourceID = MovementComponent->ApplyRootMotionSource(MoveToForce);
		}
	}
}

void UAbilityTask_DashTo::AdjustTargetLocation()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();

	FVector ActorLocation = ASC->GetAvatarActor()->GetActorLocation();
	FVector TargetLocationParallel = FVector(TargetLocation.X, TargetLocation.Y, ActorLocation.Z);
	FQuat FromTargetQuat = (ActorLocation - TargetLocationParallel).ToOrientationQuat();

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, TargetLocationParallel, FromTargetQuat, ECC_Pawn, FCollisionShape::MakeCapsule(20, 100), FCollisionQueryParams::DefaultQueryParam, ResponseParams);
	if (Overlaps.IsEmpty())
	{
		return;
	}

	// The number of the world static actor blocking should be one by the map design.
	if (const AActor* WorldStaticActor = Overlaps.Top().GetActor())
	{
		TArray<FHitResult> Hits;
		GetWorld()->SweepMultiByChannel(Hits, ActorLocation, TargetLocationParallel, FromTargetQuat/*The character should be popped back*/, ECC_Pawn, FCollisionShape::MakeCapsule(42, 100), QueryParams, ResponseParams);

		for (const FHitResult& Result : Hits)
		{
			if (Result.GetActor() == WorldStaticActor)
			{
				TargetLocation = Result.Location;
				TargetLocation.Z = 0.0f;
				UE_LOG(LogTemp, Display, TEXT("the dash should be blocked : [%s]"), *WorldStaticActor->GetName());
				break;
			}
		}
	}
}

//////////////////////////////
// UAbilityTask_DashToActor
//////////////////////////////

UAbilityTask_DashToActor::UAbilityTask_DashToActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AcceptRadius = 90.0f;
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

void UAbilityTask_DashToActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityTask_DashToActor, TargetActor);
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

			SetMovementMode();
			// Set capsule collision
			ASC->SetLooseGameplayTagCount(TAG_DASHING, 1);

			ForceName = ForceName.IsNone() ? FName("AbilityTaskDashToActor") : ForceName;
			TSharedPtr<FRootMotionSource_MoveToDynamicConstantSpeed> MoveToActorForce = MakeShared<FRootMotionSource_MoveToDynamicConstantSpeed>();
			MoveToActorForce->InstanceName = ForceName;
			MoveToActorForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			MoveToActorForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
			MoveToActorForce->Priority = 900;
			MoveToActorForce->TargetLocation = TargetLocation;
			MoveToActorForce->StartLocation = StartLocation;
			MoveToActorForce->Speed = DashSpeed;
			MoveToActorForce->AcceptRadius = AcceptRadius;
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
		// The source this task applied(SharedInitAndApply). The moves go on toward where the target is now, and the move
		// that gets there ends the dash.
		if (RMS.IsValid() && (RMS->GetScriptStruct() == FRootMotionSource_MoveToDynamicConstantSpeed::StaticStruct()))
		{
			static_cast<FRootMotionSource_MoveToDynamicConstantSpeed*>(RMS.Get())->SetTargetLocation(NewTargetLocation);
		}
	}
}