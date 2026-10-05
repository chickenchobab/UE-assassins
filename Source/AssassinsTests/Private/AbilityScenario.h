// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilityTestChecks.h"
#include "AbilityTestDriver.h"
#include "AbilityTestSession.h"
#include "AbilityTestSteps.h"

#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "GameplayTagContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

class AAssassinsChampion;
class FAutomationTestBase;
class UAbilitySystemComponent;

/**
 * What the scenario and combo tests(Assassins.Abilities.Scenario.*, Assassins.Abilities.Combo.*) are made of. The caster
 * is the first player of the session and the enemy the second, standing 300 apart as after a reset.
 */
namespace AbilityScenarios
{
	using EResult = FAbilityTestSteps::EResult;

	extern const TCHAR* Zed;
	extern const TCHAR* Akali;
	extern const TCHAR* Leblanc;

	constexpr float DamageTolerance = 0.01f;

	// No measurement yet: the damage only has to be dealt.
	constexpr float AnyDamage = -1.f;

	// How long a press that started nothing waits before the next: about as soon as a player presses again. Longer, the
	// combos left gaps no player leaves(Akali's E half a second after her R).
	constexpr double RetapSeconds = 0.1;

	struct FState
	{
		float EnemyHealthBefore = 0.f;
		float EnemyLowestHealth = 0.f;
		int32 MostProjectiles = 0;
		bool bEnemyRootedSeen = false;
		TUniquePtr<FAbilityTestRecorder> Recorder;

		// The effects the enemy takes on the server, with the abilities that sent them.
		TUniquePtr<FAbilityTestDamageLog> DamageLog;
	};

	using FGetAim = TFunction<FAbilityTestAim()>;

	FGameplayTag Input(const TCHAR* Name);
	const FAbilityTestPlayer& PlayerAt(int32 Index);
	const FAbilityTestPlayer& Caster();
	const FAbilityTestPlayer& Enemy();
	float EnemyHealth();
	bool HasTag(const UAbilitySystemComponent* AbilitySystem, const FGameplayTag& Tag);

	// Watches what the checks look back on: the enemy's lowest health and whether it was rooted, the most projectiles at
	// once on the server. Run has it done every frame.
	void Observe(FState& State);

	template <class T>
	int32 CountActors(UWorld* World)
	{
		int32 Count = 0;
		if (World)
		{
			for (TActorIterator<T> It(World); It; ++It)
			{
				++Count;
			}
		}
		return Count;
	}

	template <class T>
	T* FindActor(UWorld* World)
	{
		if (World)
		{
			for (TActorIterator<T> It(World); It; ++It)
			{
				return *It;
			}
		}
		return nullptr;
	}

	// The granted ability whose class name has the text, e.g. an ability that only an event starts.
	UClass* FindAbilityClass(const UAbilitySystemComponent* AbilitySystem, const TCHAR* ClassNamePart);

	// The granted ability the input starts.
	UClass* FindAbilityClass(const UAbilitySystemComponent* AbilitySystem, const FGameplayTag& InputTag);

	// An ability running on the ability system that is not a passive, empty when none is.
	FString FindActiveAbility(const UAbilitySystemComponent* AbilitySystem);

	// Whether the caster stands this close to the location, or to the enemy, on the server.
	bool IsCasterNear(const FVector& Location, double Tolerance);
	bool IsCasterNearEnemy(double Tolerance);

	// How far the champion stands past the target, along the way from the caster's start to the target: below zero on the
	// caster's side. Both in the same world.
	double GetDistancePast(const AActor* Champion, const AActor* Target);

	void Place(AAssassinsChampion* Champion, const FVector& FloorLocation);

	// Puts the player's champion there on the server and on its own client, then waits until the caster's client sees it.
	void MovePlayer(FAbilityTestSteps& Steps, int32 PlayerIndex, const FVector& FloorLocation);

	FAbilityTestSessionConfig Versus(const TCHAR* CasterChampion, const TCHAR* EnemyChampion, bool bFreshSession = false);

	// A test's own steps, after the session start and the reset.
	using FAddSteps = TFunction<void(FAbilityTestSteps&, TSharedRef<FState>)>;

	// The session for the champions, a reset, then the test's own steps. What the enemy took is reported at the end.
	bool Run(FAutomationTestBase& Test, const FAbilityTestSessionConfig& Config, FAddSteps AddSteps);

	// The scenarios and the combos are kept by group(Scenario, Combo). A group is a test, Assassins.Abilities.<Group>, with
	// a test for each of its scenarios(Assassins.Abilities.Scenario.Zed.R). The group runs again with lag on the clients and
	// with the caster hosting a listen server(Lag100, Lag200, Listen in front of the group). A scenario joins its group with
	// a static FScenarioRegistration in the file that has it.
	struct FScenarioRegistration
	{
		// Between the two champions, the caster first.
		FScenarioRegistration(const TCHAR* Group, const TCHAR* Name, const TCHAR* CasterChampion, const TCHAR* EnemyChampion, FAddSteps AddSteps);

		// The same, kept out of the variants when bWithVariants is false: a scenario that only looks at the server, where
		// neither lag nor a listen server changes anything.
		FScenarioRegistration(const TCHAR* Group, const TCHAR* Name, const TCHAR* CasterChampion, const TCHAR* EnemyChampion, bool bWithVariants, FAddSteps AddSteps);

		// In the session the config gives: a new one, more players.
		FScenarioRegistration(const TCHAR* Group, const TCHAR* Name, TFunction<FAbilityTestSessionConfig()> GetConfig, FAddSteps AddSteps);
	};

	// What the group's tests do: list the scenarios(for a variant, the ones that run in the variants), and run one with
	// the server and the lag of the variant.
	void GetScenarioTests(const TCHAR* Group, TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands, bool bVariant = false);
	bool RunScenarioTest(FAutomationTestBase& Test, const TCHAR* Group, const FString& Name, bool bListenServer, int32 LagMs);

	// Sideways from the caster's or the enemy's start, away from the line between them, or from the caster's start toward
	// the enemy's.
	FVector SpotBesideCaster(double Distance);
	FVector SpotBesideEnemy(double Distance);
	FVector SpotTowardEnemy(double Distance);

	FGetAim AimAtEnemy();
	FGetAim AimAtPlayer(int32 PlayerIndex);
	FGetAim AimAtSpot(const FVector& Spot);

	void TapAtEnemy(FAbilityTestSteps& Steps, const FGameplayTag& InputTag);

	// At a spot on the floor with no one under the cursor.
	void TapAtSpot(FAbilityTestSteps& Steps, const FGameplayTag& InputTag, const FVector& Spot);

	void WaitForDamage(FAbilityTestSteps& Steps, TSharedRef<FState> State, double TimeoutSeconds);

	// Watches the enemy's health for a while, then checks the damage it took since the start: some, or the expected ±1%.
	void CheckDamage(FAbilityTestSteps& Steps, TSharedRef<FState> State, double WatchSeconds, float Expected);

	// Checks the damage the ability(the input's, or the one whose class name has the text) dealt since the start: some,
	// or the expected ±1%.
	void CheckDamageFrom(FAbilityTestSteps& Steps, TSharedRef<FState> State, const FGameplayTag& InputTag, float Expected = AnyDamage);
	void CheckDamageFrom(FAbilityTestSteps& Steps, TSharedRef<FState> State, const TCHAR* ClassNamePart, float Expected = AnyDamage);

	// Waits until the input's ability has put the effect(by a part of its class name) on the enemy, on the server: one
	// part of what an ability deals, e.g. the dash of Akali's E again apart from its shuriken.
	void WaitForEffectFrom(FAbilityTestSteps& Steps, TSharedRef<FState> State, const FString& Description, const FGameplayTag& InputTag, const TCHAR* EffectNamePart, double TimeoutSeconds = 3.0);

	void WaitUntilNear(FAbilityTestSteps& Steps, const FString& Description, double TimeoutSeconds, TFunction<const AActor*()> GetActor, TFunction<FVector()> GetLocation, double Tolerance);

	void WaitForTag(FAbilityTestSteps& Steps, const FString& Description, double TimeoutSeconds, TFunction<const UAbilitySystemComponent*()> GetAbilitySystem, const FGameplayTag& Tag, bool bPresent = true);

	// Taps the input at the aim, again every RetapSeconds, until the condition holds: a recast, which may listen only after
	// a while(and for the input to be let go first).
	void TapUntil(FAbilityTestSteps& Steps, const FString& Description, const FGameplayTag& InputTag, FGetAim GetAim, double TimeoutSeconds, TFunction<bool()> Condition);
	void TapAtEnemyUntil(FAbilityTestSteps& Steps, const FString& Description, const FGameplayTag& InputTag, double TimeoutSeconds, TFunction<bool()> Condition);

	// Casts the way a player does in a combo: once the cast before is over(no Status.Channeling on the owning client),
	// presses the input at the aim, again every RetapSeconds while something keeps the ability from starting, until it
	// starts on the owning client; then waits for the server. The ability that has to start is the input's, or the one
	// whose class name has ExpectedAbility(the mimic Leblanc's R starts).
	void CastAbility(FAbilityTestSteps& Steps, const FString& Name, const FGameplayTag& InputTag, FGetAim GetAim, const TCHAR* ExpectedAbility = nullptr, double TimeoutSeconds = 5.0);

	// A basic attack at the enemy: once the cast or the dash before is over, presses the attack unless one is going on
	// (again if one ends without having hit), and waits for it to hit. The attack goes on, as it does in a game, until an
	// ability or StopAttacking stops it.
	void AttackOnce(FAbilityTestSteps& Steps, TSharedRef<FState> State, double TimeoutSeconds = 8.0);

	// Whether an attack runs on the ability system: the attack, or the ability that keeps attacking between two.
	bool IsAttacking(const UAbilitySystemComponent* AbilitySystem);

	// Cancels the attacks on the player's client: the attack and the ability that keeps attacking.
	void StopAttacking(const FAbilityTestPlayer& StoppingPlayer);

	// Zed's Q at the enemy, then checks how many shurikens flew at once on the server.
	void ThrowShurikens(FAbilityTestSteps& Steps, TSharedRef<FState> State, int32 AtLeast);

	// Waits until the caster stands past the enemy, on the far side from the caster's start(Zed after his ultimate), and
	// notes when.
	void WaitUntilBehindEnemy(FAbilityTestSteps& Steps, TSharedRef<double> OutArrivalTime);

	// The end of a combo: stops attacking, waits for every ability to end on both sides and checks that no transient tag
	// is left.
	void FinishCombo(FAbilityTestSteps& Steps, double TimeoutSeconds = 15.0);
}

#endif // WITH_DEV_AUTOMATION_TESTS
