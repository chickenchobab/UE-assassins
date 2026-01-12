// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AssassinsTargetChasingComponent.h"

UAssassinsTargetChasingComponent::UAssassinsTargetChasingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bKeepChase = false;
	bKeepChaseBefore = false;
	CachedTarget = nullptr;
	CachedAcceptRadius = 0.0f;
}

void UAssassinsTargetChasingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If bKeepChase was just set to false, allow movement for one additional tick.
	if ((bKeepChaseBefore || bKeepChase) && CachedTarget.IsValid())
	{
		ChaseTargetDelegate.ExecuteIfBound(CachedTarget.Get(), CachedAcceptRadius);
	}

	bKeepChaseBefore = bKeepChase;
}

void UAssassinsTargetChasingComponent::ChaseTarget(AActor* Target, float AcceptRadius)
{
	SetTargetState(Target, AcceptRadius);

	ChaseTargetDelegate.ExecuteIfBound(Target, AcceptRadius);
}

void UAssassinsTargetChasingComponent::StopChase()
{
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

