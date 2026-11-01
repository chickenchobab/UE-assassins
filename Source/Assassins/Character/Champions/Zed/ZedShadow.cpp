#include "Character/Champions/Zed/ZedShadow.h"

#include "Character/AssassinsChampion.h"
#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

AZedShadow::AZedShadow()
{
}

void AZedShadow::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZedShadow, ShadowLifeSpan);
}

void AZedShadow::BeginPlay()
{
	Super::BeginPlay();

	// 소유 클라이언트에 복제된 서버 그림자는 숨기기만 한다. 그 머신에서 수명과 등록은 예측해서 만든 그림자가 맡는다.
	if (ShouldSkipCosmetic())
	{
		return;
	}

	RegisterWithOwner();
	if (ShadowLifeSpan > 0.0)
	{
		GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, this, &ThisClass::StartDisappearing, static_cast<float>(ShadowLifeSpan), false);
	}
}

void AZedShadow::StartDisappearing()
{
	UnregisterFromOwner();
	OnStartDisappearing();

	if (DelayForDisappear > 0.0)
	{
		GetWorldTimerManager().SetTimer(DisappearTimerHandle, this, &ThisClass::FinishDisappearing, static_cast<float>(DelayForDisappear), false);
		return;
	}
	FinishDisappearing();
}

void AZedShadow::FinishDisappearing()
{
	// 복제된 그림자는 서버의 파괴를 받아 사라진다.
	if (HasAuthority())
	{
		Destroy();
	}
}

AZedShadow* AZedShadow::SpawnShadow(TSubclassOf<AZedShadow> ShadowClass, AActor* Zed, const FTransform& SpawnTransform, double LifeSpan)
{
	UWorld* World = Zed ? Zed->GetWorld() : nullptr;
	if ((World == nullptr) || !ShadowClass)
	{
		return nullptr;
	}

	AZedShadow* Shadow = World->SpawnActorDeferred<AZedShadow>(
		ShadowClass, SpawnTransform, Zed, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Shadow == nullptr)
	{
		return nullptr;
	}

	Shadow->ShadowLifeSpan = LifeSpan;
	Shadow->FinishSpawning(SpawnTransform);
	return Shadow;
}

void AZedShadow::SwapPlacesWith(ACharacter* Zed)
{
	if (Zed == nullptr)
	{
		return;
	}

	const FVector ShadowLocation = GetActorLocation();
	const FRotator ShadowRotation = GetActorRotation();

	SetActorLocationAndRotation(Zed->GetActorLocation(), Zed->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);

	if (UAssassinsCharacterMovementComponent* Movement = Cast<UAssassinsCharacterMovementComponent>(Zed->GetCharacterMovement()))
	{
		Movement->TeleportCharacter(ShadowLocation, ShadowRotation);
	}
}

void AZedShadow::RegisterWithOwner()
{
	if (UAssassinsChampionSkillState_Zed* ZedState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Zed>(GetOwner()))
	{
		ZedState->AddShadow(this);
	}
}

void AZedShadow::UnregisterFromOwner()
{
	if (UAssassinsChampionSkillState_Zed* ZedState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Zed>(GetOwner()))
	{
		ZedState->RemoveShadow(this);
	}
}
