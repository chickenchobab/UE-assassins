// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AssassinsTargetChasingComponent.h"
#include "Character/AssassinsCharacter.h"

UAssassinsTargetChasingComponent::UAssassinsTargetChasingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bKeepChase = false;
	bKeptChaseLastTick = false;
	CachedTarget = nullptr;
	CachedAcceptRadius = 0.0f;
}

void UAssassinsTargetChasingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If bKeepChase was just set to false, allow movement for one additional tick.
	// 추적이 어빌리티에 도착을 알린다(HandleChaseCompleted). 이미 대상에 있는 아바타는 추적이 시작돼야 다시 도착하므로,
	// 이 한 틱이 없으면 도착을 기다리는 어빌리티(이전 공격 쿨다운 중 새 대상에 닿은 공격)가 도착을 듣지 못한다.
	const bool bChase = bKeepChase || bKeptChaseLastTick;
	bKeptChaseLastTick = bKeepChase;

	if (bChase && CachedTarget.IsValid())
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
	bKeptChaseLastTick = false;

	HandleChaseCompleted.Clear();

	CachedTarget = nullptr;
	CachedAcceptRadius = 0.0f;
}

