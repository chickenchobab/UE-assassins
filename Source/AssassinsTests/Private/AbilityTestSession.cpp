// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityTestSession.h"
#include "AbilityTestSteps.h"
#include "AssassinsTestsLog.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/Champions/Akali/AssassinsShroud.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "GameModes/AssassinsExperienceDefinition.h"
#include "Player/AssassinsPlayerController.h"
#include "Player/AssassinsPlayerState.h"
#include "System/AssassinsDeveloperSettings.h"

#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"

const TCHAR* FAbilityTestSession::TestMapPackageName = TEXT("/Game/Tests/L_AbilityTest");

namespace AbilityTestSession
{
	// How far a champion may be from where it should stand.
	constexpr double LocationTolerance = 30.0;

	// PIE windows small enough to leave room for the editor.
	const FIntPoint ClientWindowSize(640, 360);

	bool IsGameplayReady(const APawn* Pawn)
	{
		const UAssassinsPawnExtensionComponent* PawnExtension = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
		return PawnExtension && PawnExtension->HasReachedInitState(AssassinsGameplayTags::InitState_GameplayReady);
	}

	AAssassinsPlayerState* FindPlayerState(const UWorld* World, int32 PlayerId)
	{
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		if (GameState == nullptr)
		{
			return nullptr;
		}

		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (PlayerState && (PlayerState->GetPlayerId() == PlayerId))
			{
				return Cast<AAssassinsPlayerState>(PlayerState);
			}
		}
		return nullptr;
	}

	bool IsPassive(const FGameplayAbilitySpec& Spec)
	{
		const UAssassinsGameplayAbility* AbilityCDO = Cast<UAssassinsGameplayAbility>(Spec.Ability);
		return AbilityCDO && (AbilityCDO->GetActivationPolicy() == EAssassinsAbilityActivationPolicy::OnSpawn);
	}

	// The name of an active ability that is not a passive, empty when there is none.
	FString FindActiveAbility(const UAbilitySystemComponent* AbilitySystem)
	{
		if (AbilitySystem)
		{
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				if (Spec.IsActive() && !IsPassive(Spec))
				{
					return GetNameSafe(Spec.Ability);
				}
			}
		}
		return FString();
	}

	// Ends the running abilities but the passives, which only start at spawn. The server's cancels reach the client; the
	// client's own ones(local only) end there.
	void CancelActiveAbilities(UAbilitySystemComponent* AbilitySystem)
	{
		TArray<FGameplayAbilitySpecHandle> ToCancel;
		for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
		{
			if (Spec.IsActive() && !IsPassive(Spec))
			{
				ToCancel.Add(Spec.Handle);
			}
		}

		for (const FGameplayAbilitySpecHandle& Handle : ToCancel)
		{
			AbilitySystem->CancelAbilityHandle(Handle);
		}
	}

	void ClearTransientTags(UAbilitySystemComponent* AbilitySystem)
	{
		FGameplayTagContainer OwnedTags;
		AbilitySystem->GetOwnedGameplayTags(OwnedTags);
		for (const FGameplayTag& Tag : OwnedTags)
		{
			if (FAbilityTestSession::GetTransientTags().HasTagExact(Tag))
			{
				UE_LOG(LogAssassinsTests, Log, TEXT("Reset: cleared %s off %s."), *Tag.ToString(), *GetNameSafe(AbilitySystem->GetOwner()));
				AbilitySystem->SetLooseGameplayTagCount(Tag, 0);
			}
		}
	}

	// The actors the abilities leave behind: shadows, shrouds, projectiles. On a client, only its own(predicted) ones;
	// the replicated ones go with the server's.
	void DestroyAbilityActors(UWorld* World, bool bOnlyLocal)
	{
		if (World == nullptr)
		{
			return;
		}

		TArray<AActor*> ToDestroy;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			const bool bAbilityActor = Actor->IsA<AAssassinsZedShadow>() || Actor->IsA<AAssassinsShroud>() || Actor->IsA<AAssassinsProjectile>();
			if (bAbilityActor && (!bOnlyLocal || (Actor->GetLocalRole() == ROLE_Authority)))
			{
				ToDestroy.Add(Actor);
			}
		}

		for (AActor* Actor : ToDestroy)
		{
			Actor->Destroy();
		}
	}

	void PlaceChampion(AAssassinsChampion* Champion, const FVector& FloorLocation, const FRotator& Facing)
	{
		if (Champion == nullptr)
		{
			return;
		}

		if (UCharacterMovementComponent* Movement = Champion->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}

		const float HalfHeight = Champion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Champion->TeleportTo(FloorLocation + FVector(0.0, 0.0, HalfHeight + 2.0), Facing, /*bIsATest*/ false, /*bNoCheck*/ true);
	}
}

FString FAbilityTestSessionConfig::ToString() const
{
	TArray<FString> Names;
	for (const FSoftObjectPath& Champion : Champions)
	{
		Names.Add(Champion.GetAssetName());
	}
	return FString::Printf(TEXT("%s, %d bots, %s, lag %d ms"), *FString::Join(Names, TEXT(" vs ")), NumBots, bListenServer ? TEXT("listen server") : TEXT("dedicated server"), LagMs);
}

UAssassinsAbilitySystemComponent* FAbilityTestPlayer::GetClientAbilitySystem() const
{
	const AAssassinsPlayerController* Controller = ClientController.Get();
	return Controller ? Controller->GetAssassinsAbilitySystemComponent() : nullptr;
}

UAssassinsAbilitySystemComponent* FAbilityTestPlayer::GetServerAbilitySystem() const
{
	const AAssassinsPlayerController* Controller = ServerController.Get();
	return Controller ? Controller->GetAssassinsAbilitySystemComponent() : nullptr;
}

AAssassinsChampion* FAbilityTestPlayer::FindClientCopyOf(const FAbilityTestPlayer& Other) const
{
	if (UWorld* World = ClientWorld.Get())
	{
		for (TActorIterator<AAssassinsChampion> It(World); It; ++It)
		{
			const APlayerState* PlayerState = It->GetPlayerState();
			if (PlayerState && (PlayerState->GetPlayerId() == Other.PlayerId))
			{
				return *It;
			}
		}
	}
	return nullptr;
}

bool FAbilityTestPlayer::IsValid() const
{
	return ClientWorld.IsValid() && ClientController.IsValid() && ClientChampion.IsValid() && ServerController.IsValid() && ServerChampion.IsValid();
}

FAbilityTestSession& FAbilityTestSession::Get()
{
	static FAbilityTestSession Session;
	return Session;
}

FPrimaryAssetId FAbilityTestSession::GetTestExperienceId()
{
	return FPrimaryAssetId(FPrimaryAssetType(UAssassinsExperienceDefinition::StaticClass()->GetFName()), FName(TEXT("B_AbilityTestExperience")));
}

FVector FAbilityTestSession::GetStartLocation(int32 PlayerIndex)
{
	return FVector(0.0, -150.0 + 300.0 * PlayerIndex, 0.0);
}

const FGameplayTagContainer& FAbilityTestSession::GetTransientTags()
{
	static const FGameplayTagContainer Tags = []
	{
		FGameplayTagContainer Result;
		Result.AddTag(AssassinsGameplayTags::Status_Channeling);
		Result.AddTag(AssassinsGameplayTags::Status_Dashing);
		Result.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Champion.Zed.SpawningShadow")));

		// The combos, but the passive's: Akali keeps it for her next attack on purpose.
		Result.AppendTags(UGameplayTagsManager::Get().RequestGameplayTagChildren(FGameplayTag::RequestGameplayTag(TEXT("Status.Combo"))));
		Result.RemoveTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Combo.Passive")));
		return Result;
	}();
	return Tags;
}

void FAbilityTestSession::AddStartSteps(FAbilityTestSteps& Steps, const FAbilityTestSessionConfig& Config)
{
	using EResult = FAbilityTestSteps::EResult;

	// Each world of a new play session makes its crowd manager before the map's nav mesh is registered, which warns. The
	// crowd manager is made again as the nav mesh comes(UCrowdManager::OnNavMeshUpdate): not a problem of the test.
	Steps.GetTest().AddExpectedMessagePlain(TEXT("Unable to find RecastNavMesh instance while trying to create UCrowdManager instance"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences, any or none*/ -1);

	Steps.Step(TEXT("Bring up the play session"), 60.0, [this, Config](FString& OutError)
	{
		if (!Config.bFreshSession && IsReadyFor(Config))
		{
			return EResult::Next;
		}

		// A session for another config goes down first. The user's is left alone.
		if (GEditor->IsPlaySessionInProgress())
		{
			if (!bOwnsPlaySession)
			{
				OutError = TEXT("a play session the tests did not start is running. Stop it and run the tests again");
				return EResult::Fail;
			}

			if (!bEndRequested)
			{
				UE_LOG(LogAssassinsTests, Display, TEXT("Ending the play session(%s) for another one(%s)."), *ActiveConfig.ToString(), *Config.ToString());
				GEditor->RequestEndPlayMap();
				bEndRequested = true;
			}
			OutError = TEXT("the previous play session is still ending");
			return EResult::Wait;
		}
		bEndRequested = false;

		if (!OpenTestMap(OutError))
		{
			return EResult::Fail;
		}

		AddHooks();
		ApplyDeveloperSettings(Config);
		RequestPlaySession(Config);
		return EResult::Next;
	});

	Steps.Step(TEXT("Wait for the players to be ready"), 120.0, [this](FString& OutWaitReason)
	{
		if (bPlayersReady)
		{
			return EResult::Next;
		}

		if (!bOwnsPlaySession || !GEditor->IsPlaySessionInProgress())
		{
			OutWaitReason = TEXT("the play session ended before the players were ready");
			return EResult::Fail;
		}

		if (!FindPlayers(OutWaitReason))
		{
			return EResult::Wait;
		}

		bPlayersReady = true;
		UE_LOG(LogAssassinsTests, Display, TEXT("Play session ready: %s."), *ActiveConfig.ToString());
		return EResult::Next;
	});

	// The game mode reads them as the match starts and as the players join, both done by now.
	Steps.Do(TEXT("Put back the developer settings"), [this]
	{
		RestoreDeveloperSettings();
	});

	Steps.Step(TEXT("Check that the players are enemies"), FAbilityTestSteps::DefaultTimeoutSeconds, [this](FString& OutError)
	{
		return CheckTeams(OutError) ? EResult::Next : EResult::Fail;
	});

	// The steps after this take the players for granted.
	Steps.Require([this](FString& OutError)
	{
		if (bPlayersReady && (Players.Num() == ActiveConfig.Champions.Num()))
		{
			return true;
		}

		OutError = TEXT("the play session went away");
		return false;
	});

	Steps.Finally([this]
	{
		RestoreDeveloperSettings();
	});
}

void FAbilityTestSession::AddResetSteps(FAbilityTestSteps& Steps)
{
	using EResult = FAbilityTestSteps::EResult;

	// A champion that died and respawned is another pawn.
	Steps.Step(TEXT("Find the players"), 30.0, [this](FString& OutWaitReason)
	{
		for (const FAbilityTestPlayer& Player : Players)
		{
			if (!Player.IsValid())
			{
				return FindPlayers(OutWaitReason) ? EResult::Next : EResult::Wait;
			}
		}
		return EResult::Next;
	});

	Steps.Do(TEXT("Reset the champions"), [this]
	{
		ResetChampions();
	});

	Steps.Step(TEXT("Wait for the champions to be back at their start"), 5.0, [this](FString& OutWaitReason)
	{
		return AreChampionsAtStart(OutWaitReason) ? EResult::Next : EResult::Wait;
	});

	Steps.WaitFor(TEXT("Let the reset replicate"), 0.5);
}

void FAbilityTestSession::Shutdown()
{
	if (RestoreLevelTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RestoreLevelTickerHandle);
		RestoreLevelTickerHandle.Reset();
	}

	RemoveHooks();
	RestoreDeveloperSettings();
	PlaySettings.Reset();
	Players.Reset();
}

bool FAbilityTestSession::IsReadyFor(const FAbilityTestSessionConfig& Config) const
{
	if (!bOwnsPlaySession || !bPlayersReady || !(ActiveConfig == Config) || !GEditor->IsPlayingSessionInEditor())
	{
		return false;
	}

	for (const FAbilityTestPlayer& Player : Players)
	{
		if (!Player.IsValid())
		{
			return false;
		}
	}
	return true;
}

bool FAbilityTestSession::OpenTestMap(FString& OutError)
{
	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	const FString CurrentMap = EditorWorld ? EditorWorld->GetOutermost()->GetName() : FString();
	if (CurrentMap == TestMapPackageName)
	{
		return true;
	}

	if (!FPackageName::DoesPackageExist(TestMapPackageName))
	{
		OutError = FString::Printf(TEXT("the test map %s is missing"), TestMapPackageName);
		return false;
	}

	// Opening a map drops what the open level did not save, without asking.
	TArray<UPackage*> DirtyPackages;
	FEditorFileUtils::GetDirtyWorldPackages(DirtyPackages);
	if (DirtyPackages.Num() > 0)
	{
		OutError = FString::Printf(TEXT("the open level has unsaved changes(%s). Save them and run the tests again: the tests open their own map"), *DirtyPackages[0]->GetName());
		return false;
	}

	if (!FPackageName::DoesPackageExist(CurrentMap))
	{
		OutError = TEXT("the open level was never saved. Save it and run the tests again: the tests open their own map");
		return false;
	}

	if (OriginalMapPackageName.IsEmpty())
	{
		OriginalMapPackageName = CurrentMap;
	}

	if (!FEditorFileUtils::LoadMap(TestMapPackageName, /*LoadAsTemplate*/ false, /*bShowProgress*/ false))
	{
		OutError = FString::Printf(TEXT("could not open the test map %s"), TestMapPackageName);
		return false;
	}
	return true;
}

void FAbilityTestSession::ApplyDeveloperSettings(const FAbilityTestSessionConfig& Config)
{
	UAssassinsDeveloperSettings* Settings = GetMutableDefault<UAssassinsDeveloperSettings>();

	if (!SavedDeveloperSettings.IsSet())
	{
		FSavedDeveloperSettings& Saved = SavedDeveloperSettings.Emplace();
		Saved.ExperienceOverride = Settings->ExperienceOverride;
		Saved.ChampionOverrides = Settings->ChampionOverrides;
		Saved.NumBotsOverride = Settings->NumBotsOverride;
	}

	// In memory only: nothing is saved to the user's config.
	Settings->ExperienceOverride = GetTestExperienceId();
	Settings->ChampionOverrides.Reset();
	for (const FSoftObjectPath& Champion : Config.Champions)
	{
		Settings->ChampionOverrides.Add(TSoftObjectPtr<UAssassinsPawnData>(Champion));
	}
	Settings->NumBotsOverride = Config.NumBots;
}

void FAbilityTestSession::RestoreDeveloperSettings()
{
	if (!SavedDeveloperSettings.IsSet())
	{
		return;
	}

	UAssassinsDeveloperSettings* Settings = GetMutableDefault<UAssassinsDeveloperSettings>();
	Settings->ExperienceOverride = SavedDeveloperSettings->ExperienceOverride;
	Settings->ChampionOverrides = SavedDeveloperSettings->ChampionOverrides;
	Settings->NumBotsOverride = SavedDeveloperSettings->NumBotsOverride;
	SavedDeveloperSettings.Reset();
}

void FAbilityTestSession::RequestPlaySession(const FAbilityTestSessionConfig& Config)
{
	// A new object leaves the user's play settings alone. It starts from them, so everything that matters is set.
	PlaySettings = TStrongObjectPtr<ULevelEditorPlaySettings>(NewObject<ULevelEditorPlaySettings>());
	// A listen server's host is one of the players.
	PlaySettings->SetPlayNetMode(Config.bListenServer ? EPlayNetMode::PIE_ListenServer : EPlayNetMode::PIE_Client);
	PlaySettings->SetPlayNumberOfClients(Config.Champions.Num());
	PlaySettings->SetRunUnderOneProcess(true);
	PlaySettings->bLaunchSeparateServer = false;
	PlaySettings->GameGetsMouseControl = false;

	// The round trip split between the two ways, on the clients only.
	FLevelEditorPlayNetworkEmulationSettings& Emulation = PlaySettings->NetworkEmulationSettings;
	Emulation.bIsNetworkEmulationEnabled = Config.LagMs > 0;
	Emulation.EmulationTarget = NetworkEmulationTarget::Client;
	for (FNetworkEmulationPacketSettings* Packets : { &Emulation.OutPackets, &Emulation.InPackets })
	{
		Packets->MinLatency = Config.LagMs / 2;
		Packets->MaxLatency = Config.LagMs / 2;
		Packets->PacketLossPercentage = 0;
	}
	PlaySettings->NewWindowWidth = AbilityTestSession::ClientWindowSize.X;
	PlaySettings->NewWindowHeight = AbilityTestSession::ClientWindowSize.Y;
	PlaySettings->SetClientWindowSize(AbilityTestSession::ClientWindowSize);

	FRequestPlaySessionParams Params;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	Params.EditorPlaySettings = PlaySettings.Get();

	UE_LOG(LogAssassinsTests, Display, TEXT("Starting a play session: %s."), *Config.ToString());
	GEditor->RequestPlaySession(Params);

	bOwnsPlaySession = true;
	bPlayersReady = false;
	ActiveConfig = Config;
	Players.Reset();
	ServerWorld.Reset();
}

bool FAbilityTestSession::FindPlayers(FString& OutWaitReason)
{
	UWorld* Server = nullptr;
	TArray<UWorld*> Clients;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if ((Context.WorldType != EWorldType::PIE) || (World == nullptr))
		{
			continue;
		}

		// A listen server's world is its host's too: the host plays there.
		const ENetMode NetMode = World->GetNetMode();
		if ((NetMode == NM_DedicatedServer) || (NetMode == NM_ListenServer))
		{
			Server = World;
		}
		if (NetMode != NM_DedicatedServer)
		{
			Clients.Add(World);
		}
	}

	if (Server == nullptr)
	{
		OutWaitReason = TEXT("no server world yet");
		return false;
	}

	if (Clients.Num() < ActiveConfig.Champions.Num())
	{
		OutWaitReason = FString::Printf(TEXT("%d of %d client worlds"), Clients.Num(), ActiveConfig.Champions.Num());
		return false;
	}

	TArray<FAbilityTestPlayer> Found;
	for (UWorld* ClientWorld : Clients)
	{
		// The world's own player: on a listen server, the others' controllers are there too.
		AAssassinsPlayerController* ClientController = nullptr;
		for (FConstPlayerControllerIterator It = ClientWorld->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Controller = It->Get();
			if (Controller && Controller->IsLocalController())
			{
				ClientController = Cast<AAssassinsPlayerController>(Controller);
				break;
			}
		}
		const AAssassinsPlayerState* ClientPlayerState = ClientController ? ClientController->GetPlayerState<AAssassinsPlayerState>() : nullptr;
		AAssassinsChampion* ClientChampion = ClientController ? Cast<AAssassinsChampion>(ClientController->GetPawn()) : nullptr;
		if ((ClientPlayerState == nullptr) || (ClientChampion == nullptr) || !AbilityTestSession::IsGameplayReady(ClientChampion))
		{
			OutWaitReason = FString::Printf(TEXT("the champion of %s is not ready on its client"), *ClientWorld->GetName());
			return false;
		}

		const UAssassinsPawnData* PawnData = ClientPlayerState->GetPawnData<UAssassinsPawnData>();
		AAssassinsPlayerState* ServerPlayerState = AbilityTestSession::FindPlayerState(Server, ClientPlayerState->GetPlayerId());
		AAssassinsChampion* ServerChampion = ServerPlayerState ? Cast<AAssassinsChampion>(ServerPlayerState->GetPawn()) : nullptr;
		AAssassinsPlayerController* ServerController = ServerPlayerState ? Cast<AAssassinsPlayerController>(ServerPlayerState->GetOwningController()) : nullptr;
		if ((PawnData == nullptr) || (ServerChampion == nullptr) || (ServerController == nullptr) || !AbilityTestSession::IsGameplayReady(ServerChampion))
		{
			OutWaitReason = FString::Printf(TEXT("the champion of %s is not ready on the server"), *ClientWorld->GetName());
			return false;
		}

		// The abilities the server granted have come to the client.
		const UAbilitySystemComponent* ClientAbilitySystem = ClientController->GetAssassinsAbilitySystemComponent();
		const UAbilitySystemComponent* ServerAbilitySystem = ServerController->GetAssassinsAbilitySystemComponent();
		if ((ClientAbilitySystem == nullptr) || (ServerAbilitySystem == nullptr) ||
			(ClientAbilitySystem->GetActivatableAbilities().Num() < ServerAbilitySystem->GetActivatableAbilities().Num()))
		{
			OutWaitReason = FString::Printf(TEXT("the abilities of %s have not come to its client"), *GetNameSafe(PawnData));
			return false;
		}

		FAbilityTestPlayer& Player = Found.AddDefaulted_GetRef();
		Player.PawnData = FSoftObjectPath(PawnData);
		Player.PlayerId = ClientPlayerState->GetPlayerId();
		Player.ClientWorld = ClientWorld;
		Player.ClientController = ClientController;
		Player.ClientChampion = ClientChampion;
		Player.ServerController = ServerController;
		Player.ServerChampion = ServerChampion;
	}

	// Every client has the champions of the others.
	for (const FAbilityTestPlayer& Viewer : Found)
	{
		for (const FAbilityTestPlayer& Other : Found)
		{
			if ((&Viewer != &Other) && (Viewer.FindClientCopyOf(Other) == nullptr))
			{
				OutWaitReason = FString::Printf(TEXT("%s has not come to the client of %s"), *Other.PawnData.GetAssetName(), *Viewer.PawnData.GetAssetName());
				return false;
			}
		}
	}

	// In the order of the config. Who plays which champion follows who joined first, so it goes by the pawn data.
	TArray<FAbilityTestPlayer> Ordered;
	for (const FSoftObjectPath& Champion : ActiveConfig.Champions)
	{
		const int32 FoundIndex = Found.IndexOfByPredicate([&Champion](const FAbilityTestPlayer& Player) { return Player.PawnData == Champion; });
		if (FoundIndex == INDEX_NONE)
		{
			OutWaitReason = FString::Printf(TEXT("no player plays %s"), *Champion.GetAssetName());
			return false;
		}

		Ordered.Add(Found[FoundIndex]);
		Found.RemoveAt(FoundIndex);
	}

	Players = MoveTemp(Ordered);
	ServerWorld = Server;
	return true;
}

bool FAbilityTestSession::CheckTeams(FString& OutError) const
{
	if (Players.Num() < 2)
	{
		return true;
	}

	const AAssassinsPlayerState* First = Players[0].ServerController.IsValid() ? Players[0].ServerController->GetPlayerState<AAssassinsPlayerState>() : nullptr;
	const AAssassinsPlayerState* Second = Players[1].ServerController.IsValid() ? Players[1].ServerController->GetPlayerState<AAssassinsPlayerState>() : nullptr;
	if ((First == nullptr) || (Second == nullptr))
	{
		OutError = TEXT("the players have no player state on the server");
		return false;
	}

	const int32 FirstTeam = GenericTeamIdToInteger(First->GetGenericTeamId());
	const int32 SecondTeam = GenericTeamIdToInteger(Second->GetGenericTeamId());
	if ((FirstTeam == INDEX_NONE) || (FirstTeam == SecondTeam))
	{
		OutError = FString::Printf(TEXT("the players are on teams %d and %d. Bots take team slots too"), FirstTeam, SecondTeam);
		return false;
	}
	return true;
}

void FAbilityTestSession::ResetChampions()
{
	const FGameplayAttribute HealthAttribute = UAssassinsHealthSet::GetHealthAttribute();
	const FGameplayAttribute MaxHealthAttribute = UAssassinsHealthSet::GetMaxHealthAttribute();

	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		const FAbilityTestPlayer& Player = Players[Index];
		UAssassinsAbilitySystemComponent* ServerAbilitySystem = Player.GetServerAbilitySystem();
		UAssassinsAbilitySystemComponent* ClientAbilitySystem = Player.GetClientAbilitySystem();
		if ((ServerAbilitySystem == nullptr) || (ClientAbilitySystem == nullptr))
		{
			continue;
		}

		AbilityTestSession::CancelActiveAbilities(ServerAbilitySystem);
		AbilityTestSession::CancelActiveAbilities(ClientAbilitySystem);

		// The timed effects: cooldowns, buffs, crowd control. The infinite ones hold the champion's stats.
		FGameplayEffectQuery TimedEffects;
		TimedEffects.CustomMatchDelegate.BindLambda([](const FActiveGameplayEffect& Effect)
		{
			return Effect.GetDuration() > 0.f;
		});
		ServerAbilitySystem->RemoveActiveEffects(TimedEffects);

		ServerAbilitySystem->SetNumericAttributeBase(HealthAttribute, ServerAbilitySystem->GetNumericAttribute(MaxHealthAttribute));

		// Loose tags are each machine's own.
		AbilityTestSession::ClearTransientTags(ServerAbilitySystem);
		AbilityTestSession::ClearTransientTags(ClientAbilitySystem);

		// On the server and the owning client at once, so that no correction has to come.
		const FVector Start = GetStartLocation(Index);
		const FVector LookAt = GetStartLocation(Index == 0 ? 1 : 0);
		const FRotator Facing = (LookAt - Start).GetSafeNormal2D().Rotation();
		AbilityTestSession::PlaceChampion(Player.ServerChampion.Get(), Start, Facing);
		AbilityTestSession::PlaceChampion(Player.ClientChampion.Get(), Start, Facing);

		for (AAssassinsPlayerController* Controller : { Player.ServerController.Get(), Player.ClientController.Get() })
		{
			if (Controller)
			{
				Controller->StopMovement();
			}
		}
	}

	AbilityTestSession::DestroyAbilityActors(ServerWorld.Get(), /*bOnlyLocal*/ false);
	for (const FAbilityTestPlayer& Player : Players)
	{
		if (Player.ClientWorld != ServerWorld)
		{
			AbilityTestSession::DestroyAbilityActors(Player.ClientWorld.Get(), /*bOnlyLocal*/ true);
		}
	}
}

bool FAbilityTestSession::AreChampionsAtStart(FString& OutWaitReason) const
{
	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		const FAbilityTestPlayer& Player = Players[Index];
		const FVector Start = GetStartLocation(Index);

		// The champion on the server, on its own client and on the other clients.
		TArray<TPair<FString, const AAssassinsChampion*>> Copies;
		Copies.Emplace(TEXT("the server"), Player.ServerChampion.Get());
		for (const FAbilityTestPlayer& Viewer : Players)
		{
			const AAssassinsChampion* Copy = (&Viewer == &Player) ? Player.ClientChampion.Get() : Viewer.FindClientCopyOf(Player);
			Copies.Emplace(FString::Printf(TEXT("the client of %s"), *Viewer.PawnData.GetAssetName()), Copy);
		}

		for (const TPair<FString, const AAssassinsChampion*>& Copy : Copies)
		{
			if (Copy.Value == nullptr)
			{
				OutWaitReason = FString::Printf(TEXT("%s is missing on %s"), *Player.PawnData.GetAssetName(), *Copy.Key);
				return false;
			}

			const double Distance = FVector::Dist2D(Copy.Value->GetActorLocation(), Start);
			if (Distance > AbilityTestSession::LocationTolerance)
			{
				OutWaitReason = FString::Printf(TEXT("%s is %.0f away from its start on %s"), *Player.PawnData.GetAssetName(), Distance, *Copy.Key);
				return false;
			}
		}

		for (const UAbilitySystemComponent* AbilitySystem : { static_cast<const UAbilitySystemComponent*>(Player.GetServerAbilitySystem()), static_cast<const UAbilitySystemComponent*>(Player.GetClientAbilitySystem()) })
		{
			const FString ActiveAbility = AbilityTestSession::FindActiveAbility(AbilitySystem);
			if (!ActiveAbility.IsEmpty())
			{
				OutWaitReason = FString::Printf(TEXT("%s is still active on %s"), *ActiveAbility, *GetNameSafe(AbilitySystem ? AbilitySystem->GetWorld() : nullptr));
				return false;
			}
		}
	}
	return true;
}

void FAbilityTestSession::AddHooks()
{
	// A restore still waiting from the last run: this run restores at its end instead.
	if (RestoreLevelTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RestoreLevelTickerHandle);
		RestoreLevelTickerHandle.Reset();
	}

	if (bHooksAdded)
	{
		return;
	}

	EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FAbilityTestSession::HandleEndPIE);
	AfterAllTestsHandle = FAutomationTestFramework::Get().OnAfterAllTestsEvent.AddRaw(this, &FAbilityTestSession::HandleAfterAllTests);
	bHooksAdded = true;
}

void FAbilityTestSession::RemoveHooks()
{
	if (!bHooksAdded)
	{
		return;
	}

	FEditorDelegates::EndPIE.Remove(EndPIEHandle);
	FAutomationTestFramework::Get().OnAfterAllTestsEvent.Remove(AfterAllTestsHandle);
	bHooksAdded = false;
}

void FAbilityTestSession::HandleEndPIE(const bool bIsSimulating)
{
	bOwnsPlaySession = false;
	bPlayersReady = false;
	Players.Reset();
	ServerWorld.Reset();
	RestoreDeveloperSettings();
}

void FAbilityTestSession::HandleAfterAllTests()
{
	RestoreDeveloperSettings();

	if (bOwnsPlaySession && GEditor->IsPlaySessionInProgress())
	{
		GEditor->RequestEndPlayMap();
	}

	if (!RestoreLevelTickerHandle.IsValid())
	{
		RestoreLevelTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FAbilityTestSession::TickRestoreLevel));
	}
}

bool FAbilityTestSession::TickRestoreLevel(float DeltaTime)
{
	// The level can only change once the play session is gone.
	if (GEditor->IsPlaySessionInProgress())
	{
		return true;
	}

	RestoreLevelTickerHandle.Reset();
	RemoveHooks();
	PlaySettings.Reset();

	const FString MapToOpen = MoveTemp(OriginalMapPackageName);
	OriginalMapPackageName.Reset();

	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	const bool bTestMapOpen = EditorWorld && (EditorWorld->GetOutermost()->GetName() == TestMapPackageName);
	if (!MapToOpen.IsEmpty() && bTestMapOpen)
	{
		UE_LOG(LogAssassinsTests, Display, TEXT("Reopening %s after the tests."), *MapToOpen);
		FEditorFileUtils::LoadMap(MapToOpen, /*LoadAsTemplate*/ false, /*bShowProgress*/ false);
	}
	return false;
}
