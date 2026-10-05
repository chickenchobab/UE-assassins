#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"

#include "AbilitySystem/AssassinsProjectile.h"
#include "Character/AssassinsChampion.h"
#include "Components/CapsuleComponent.h"

namespace AkaliUltimate
{
	static constexpr float CapsuleRadius = 88.0f;
	static constexpr float CapsuleHalfHeight = 96.0f;
};

void UAssassinsChampionSkillState_Akali::Initialize()
{
	AAssassinsChampion* Champion = GetChampion();
	if (Champion == nullptr)
	{
		return;
	}

	// 스킬 상태처럼 기기마다 따로 만든다. 서버는 명중을 찾고 소유 클라이언트는 명중을 미리 본다. 폰만 겹치고, 어빌리티가 붙들 때만 켠다(AcquireUltimateCapsule).
	UltimateCapsule = NewObject<UCapsuleComponent>(Champion, TEXT("UltimateCapsule"));
	UltimateCapsule->InitCapsuleSize(AkaliUltimate::CapsuleRadius, AkaliUltimate::CapsuleHalfHeight);
	UltimateCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UltimateCapsule->SetCollisionObjectType(ECC_WorldDynamic);
	UltimateCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	UltimateCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	UltimateCapsule->SetGenerateOverlapEvents(true);
	UltimateCapsule->SetCanEverAffectNavigation(false);
	UltimateCapsule->SetHiddenInGame(true);
	UltimateCapsule->SetupAttachment(Champion->GetRootComponent());
	UltimateCapsule->RegisterComponent();
}

UCapsuleComponent* UAssassinsChampionSkillState_Akali::AcquireUltimateCapsule(const UObject* User)
{
	if (UltimateCapsule == nullptr)
	{
		return nullptr;
	}

	UltimateCapsuleUsers.Add(User);

	UltimateCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UltimateCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	return UltimateCapsule;
}

void UAssassinsChampionSkillState_Akali::ReleaseUltimateCapsule(const UObject* User)
{
	UltimateCapsuleUsers.Remove(User);

	for (auto It = UltimateCapsuleUsers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (UltimateCapsuleUsers.IsEmpty() && UltimateCapsule)
	{
		UltimateCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
