// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AssassinsTargetChasingComponent.h"
#include "Character/AssassinsCharacter.h"

UAssassinsTargetChasingComponent::UAssassinsTargetChasingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bKeepChase = false;
	CachedTarget = nullptr;
	CachedAcceptRadius = 0.0f;
}

void UAssassinsTargetChasingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If bKeepChase was just set to false, allow movement for one additional tick.
	if (bKeepChase && CachedTarget.IsValid())
	{
		ChaseTargetDelegate.ExecuteIfBound(CachedTarget.Get(), CachedAcceptRadius);
	}
}

void UAssassinsTargetChasingComponent::ChaseTarget(AActor* Target, float AcceptRadius)
{
	if (Target && !Target->IsA<AAssassinsCharacter>())
	{
		AcceptRadius += Target->GetSimpleCollisionRadius();
	}

	SetTargetState(Target, AcceptRadius);

	ChaseTargetDelegate.ExecuteIfBound(Target, AcceptRadius);
}

void UAssassinsTargetChasingComponent::StopChase()
{
	ResetTargetState();
	StopChaseDelegate.ExecuteIfBound();
}

void UAssassinsTargetChasingComponent::SetTargetState(AActor* Target, float AcceptRadius)
{
	CachedTarget = Target;
	CachedAcceptRadius = AcceptRadius;
}

void UAssassinsTargetChasingComponent::ResetTargetState()
{
	bKeepChase = false;

	HandleChaseCompleted.Clear();

	CachedTarget = nullptr;
	CachedAcceptRadius = 0.0f;
}

