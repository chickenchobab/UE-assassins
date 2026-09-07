// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/AssassinsTeamBaseActor.h"
#include "AbilitySystem/AssassinsAbilitySet.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "Character/AssassinsHealthComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "AssassinsLogCategories.h"

namespace
{
	// Me: A starting point sized for a dome shaped base. Tuned per base in the blueprint.
	constexpr float DefaultBaseVolumeRadius = 300.0f;
}

AAssassinsTeamBaseActor::AAssassinsTeamBaseActor()
{
	TeamId = INDEX_NONE;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	CollisionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionVolume"));
	SetRootComponent(CollisionVolume);
	CollisionVolume->SetMobility(EComponentMobility::Static);
	CollisionVolume->SetSphereRadius(DefaultBaseVolumeRadius);
	CollisionVolume->SetCollisionProfileName(TEXT("AssassinsStructure"));
	CollisionVolume->SetGenerateOverlapEvents(true);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent0"));
	SkeletalMeshComponent->SetupAttachment(CollisionVolume);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

	MinionSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("MinionSpawnPoint"));
	MinionSpawnPoint->SetupAttachment(CollisionVolume);

	AbilitySystemComponent = CreateDefaultSubobject<UAssassinsAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	HealthSet = CreateDefaultSubobject<UAssassinsHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UAssassinsCombatSet>(TEXT("CombatSet"));

	HealthComponent = CreateDefaultSubobject<UAssassinsHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::HandleDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::HandleDeathFinished);
}

void AAssassinsTeamBaseActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AAssassinsTeamBaseActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	AbilitySystemComponent->AddLooseGameplayTags(BaseOwnedTags);

	if (AbilitySet)
	{
		FAssassinsAbilitySet_GrantedHandles Handles;
		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &Handles);
	}

	check(HealthComponent);
	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
}

void AAssassinsTeamBaseActor::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);

	Super::BeginPlay();
}

void AAssassinsTeamBaseActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* AAssassinsTeamBaseActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAssassinsTeamBaseActor::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogAssassinsTeams, Error, TEXT("The team ID of a team base (%s) is authored in the level and can't be set at runtime"), *GetPathNameSafe(this));
}

FGenericTeamId AAssassinsTeamBaseActor::GetGenericTeamId() const
{
	return IntegerToGenericTeamId(TeamId);
}

FTransform AAssassinsTeamBaseActor::GetMinionSpawnTransform() const
{
	return MinionSpawnPoint ? MinionSpawnPoint->GetComponentTransform() : GetActorTransform();
}

void AAssassinsTeamBaseActor::ReturnAllPlayersToFrontend(float DelaySeconds)
{
	// Me: The death delegates this is expected to be bound to fire on every machine, since the
	// health component replicates its death state. Only the server acts on it, quietly.
	if (!HasAuthority())
	{
		return;
	}

	// Me: The death flow can ask for this more than once, but the players only leave once.
	if (GetWorldTimerManager().IsTimerActive(ReturnToFrontendTimerHandle))
	{
		return;
	}

	if (DelaySeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ReturnToFrontendTimerHandle, this, &ThisClass::DoReturnAllPlayersToFrontend, DelaySeconds, false);
		return;
	}

	DoReturnAllPlayersToFrontend();
}

void AAssassinsTeamBaseActor::DoReturnAllPlayersToFrontend()
{
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (!ensure(GameMode))
	{
		return;
	}

	GameMode->ReturnToMainMenuHost();
}

void AAssassinsTeamBaseActor::HandleDeathStarted()
{
	check(CollisionVolume);
	CollisionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AAssassinsTeamBaseActor::HandleDeathFinished()
{
	// Me: The base is kept in the world after being destroyed. Ending the match is handled elsewhere.
}
