// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AssassinsHeroComponent.h"

#include "Player/AssassinsPlayerState.h"
#include "Player/AssassinsPlayerController.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/AssassinsPawnData.h"
#include "AssassinsGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"

const FName UAssassinsHeroComponent::NAME_ActorFeatureName("Hero");

void UAssassinsHeroComponent::OnRegister()
{
	Super::OnRegister();;

	// It should be added to a blueprint whose base class is a Pawn.
	if (GetPawn<APawn>()) 
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

void UAssassinsHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UAssassinsPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(AssassinsGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UAssassinsHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

bool UAssassinsHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == AssassinsGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == AssassinsGameplayTags::InitState_Spawned && DesiredState == AssassinsGameplayTags::InitState_DataAvailable)
	{
		// The player state is required.
		if (!GetPlayerState<AAssassinsPlayerState>())
		{
			return false;
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			AAssassinsPlayerController* AssassinsPC = GetController<AAssassinsPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !AssassinsPC || !AssassinsPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == AssassinsGameplayTags::InitState_DataAvailable && DesiredState == AssassinsGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>();

		return AssassinsPS && Manager->HasFeatureReachedInitState(Pawn, UAssassinsPawnExtensionComponent::NAME_ActorFeatureName, AssassinsGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == AssassinsGameplayTags::InitState_DataInitialized && DesiredState == AssassinsGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UAssassinsHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == AssassinsGameplayTags::InitState_DataAvailable && DesiredState == AssassinsGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>();
		if (!ensure(Pawn && AssassinsPS))
		{
			return;
		}

		const UAssassinsPawnData* PawnData = nullptr;

		if (UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<UAssassinsPawnData>();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(AssassinsPS->GetAssassinsAbilitySystemComponent(), AssassinsPS);
		}
	}
}

void UAssassinsHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UAssassinsPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == AssassinsGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UAssassinsHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { AssassinsGameplayTags::InitState_Spawned, AssassinsGameplayTags::InitState_DataAvailable, AssassinsGameplayTags::InitState_DataInitialized, AssassinsGameplayTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}
