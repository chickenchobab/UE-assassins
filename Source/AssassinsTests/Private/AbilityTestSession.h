// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"

class AAssassinsChampion;
class AAssassinsPlayerController;
class FAbilityTestSteps;
class ULevelEditorPlaySettings;
class UAssassinsAbilitySystemComponent;
class UAssassinsPawnData;
class UWorld;

/** Who plays which champion in the play session. */
struct FAbilityTestSessionConfig
{
	// The pawn data of the players' champions, one client each.
	TArray<FSoftObjectPath> Champions;

	// Bots the server adds(UAssassinsDeveloperSettings::NumBotsOverride). They pick their own champions.
	int32 NumBots = 0;

	// The first player hosts a listen server, in place of a dedicated server.
	bool bListenServer = false;

	// Round trip lag of the clients(network emulation), 0 for none.
	int32 LagMs = 0;

	// Starts a new play session even when the running one matches, for a test that needs the skill states as they start.
	bool bFreshSession = false;

	bool operator==(const FAbilityTestSessionConfig& Other) const
	{
		return (Champions == Other.Champions) && (NumBots == Other.NumBots) && (bListenServer == Other.bListenServer) && (LagMs == Other.LagMs);
	}

	FString ToString() const;
};

/** A player of the session: its champion as its own client has it, and as the server has it. */
struct FAbilityTestPlayer
{
	FSoftObjectPath PawnData;
	int32 PlayerId = INDEX_NONE;

	TWeakObjectPtr<UWorld> ClientWorld;
	TWeakObjectPtr<AAssassinsPlayerController> ClientController;
	TWeakObjectPtr<AAssassinsChampion> ClientChampion;

	TWeakObjectPtr<AAssassinsPlayerController> ServerController;
	TWeakObjectPtr<AAssassinsChampion> ServerChampion;

	UAssassinsAbilitySystemComponent* GetClientAbilitySystem() const;
	UAssassinsAbilitySystemComponent* GetServerAbilitySystem() const;

	// The other player's champion as this player's client has it: what this player aims at.
	AAssassinsChampion* FindClientCopyOf(const FAbilityTestPlayer& Other) const;

	bool IsValid() const;
};

/**
 * FAbilityTestSession
 *
 * The play session the ability tests run in: a dedicated server and a client per champion, in the editor process, on the
 * test map with the test experience. Tests with the same config share it, and it goes down after the last test.
 *
 * Before the first test it checks the level open in the editor: the test map replaces it, and it comes back after the
 * last test. The developer settings the session plays with are set in memory only, and put back as soon as the players
 * are in.
 */
class FAbilityTestSession
{
public:
	static FAbilityTestSession& Get();

	// Adds the steps that make the session ready for the config, reusing the running one when it matches.
	void AddStartSteps(FAbilityTestSteps& Steps, const FAbilityTestSessionConfig& Config);

	// Adds the steps that bring the champions back to where they start: abilities, timed effects, health, leftover tags
	// and actors, positions.
	void AddResetSteps(FAbilityTestSteps& Steps);

	// The players in the order of the config.
	const TArray<FAbilityTestPlayer>& GetPlayers() const { return Players; }

	// The config of the running session.
	const FAbilityTestSessionConfig& GetActiveConfig() const { return ActiveConfig; }
	UWorld* GetServerWorld() const { return ServerWorld.Get(); }

	// Where a player stands after a reset, by its order in the config: facing each other, 300 apart.
	static FVector GetStartLocation(int32 PlayerIndex);

	// The loose tags the abilities put on and must take off: a reset clears them, the checks look for them.
	static const FGameplayTagContainer& GetTransientTags();

	// Unhooks everything, for the module shutdown.
	void Shutdown();

	// The map and the experience the session plays: a floor with a nav mesh, the team bases and player starts, and the
	// default experience without the minions.
	static const TCHAR* TestMapPackageName;
	static FPrimaryAssetId GetTestExperienceId();

private:
	FAbilityTestSession() = default;

	bool IsReadyFor(const FAbilityTestSessionConfig& Config) const;
	bool OpenTestMap(FString& OutError);
	void ApplyDeveloperSettings(const FAbilityTestSessionConfig& Config);
	void RestoreDeveloperSettings();
	void RequestPlaySession(const FAbilityTestSessionConfig& Config);
	bool FindPlayers(FString& OutWaitReason);
	bool CheckTeams(FString& OutError) const;

	void ResetChampions();
	bool AreChampionsAtStart(FString& OutWaitReason) const;

	void AddHooks();
	void RemoveHooks();
	void HandleEndPIE(const bool bIsSimulating);
	void HandleAfterAllTests();
	bool TickRestoreLevel(float DeltaTime);

	struct FSavedDeveloperSettings
	{
		FPrimaryAssetId ExperienceOverride;
		TArray<TSoftObjectPtr<UAssassinsPawnData>> ChampionOverrides;
		int32 NumBotsOverride = 0;
	};

	// The user's, while the session plays with its own.
	TOptional<FSavedDeveloperSettings> SavedDeveloperSettings;

	// The level the user had open, to reopen after the last test.
	FString OriginalMapPackageName;

	FAbilityTestSessionConfig ActiveConfig;
	TStrongObjectPtr<ULevelEditorPlaySettings> PlaySettings;
	TArray<FAbilityTestPlayer> Players;
	TWeakObjectPtr<UWorld> ServerWorld;

	// Whether the running play session is the tests'. The user's is left alone.
	bool bOwnsPlaySession = false;
	bool bPlayersReady = false;
	bool bEndRequested = false;

	bool bHooksAdded = false;
	FDelegateHandle EndPIEHandle;
	FDelegateHandle AfterAllTestsHandle;
	FTSTicker::FDelegateHandle RestoreLevelTickerHandle;
};
