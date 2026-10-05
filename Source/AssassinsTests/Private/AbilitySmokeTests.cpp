// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityScenario.h"
#include "AbilityTestChecks.h"
#include "AbilityTestDriver.h"
#include "AbilityTestSession.h"
#include "AbilityTestSteps.h"
#include "AssassinsTestsLog.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySet.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"
#include "Character/AssassinsPawnData.h"
#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"

#include "Engine/AssetManager.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Assassins.Abilities.Smoke
 *
 * A test for every ability a champion's ability sets give an input to, and for every passive. The champion is the first
 * player; the next champion in name order is the second, the enemy it aims at. The test taps the input and checks that
 * the ability activates on the owning client and the server, ends on both(an attack after it hits, as a move order would
 * stop it), puts its cooldown on and leaves none of the transient tags. It reports the damage the enemy took(a warning
 * when an ability with damage effects dealt none) without judging it. The abilities that only events start(recasts,
 * mimics) are for the scenario tests.
 *
 * It runs once, without lag: the scenarios and the combos cast the same abilities with lag and with a listen server too,
 * and check them more closely.
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAssassinsAbilitySmokeTest, "Assassins.Abilities.Smoke", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace AbilitySmokeTests
{
	const TCHAR* PassiveTest = TEXT("Passive");

	// The champions: the pawn data the asset manager knows of, the same the bots pick from.
	TArray<FSoftObjectPath> FindChampions()
	{
		TArray<FPrimaryAssetId> AssetIds;
		UAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(UAssassinsPawnData::StaticClass()->GetFName()), AssetIds);

		TArray<FSoftObjectPath> Champions;
		for (const FPrimaryAssetId& AssetId : AssetIds)
		{
			const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(AssetId);
			if (Path.IsValid())
			{
				Champions.Add(Path);
			}
		}

		Champions.Sort([](const FSoftObjectPath& A, const FSoftObjectPath& B) { return A.GetAssetName() < B.GetAssetName(); });
		return Champions;
	}

	// Data_Zed -> Zed.
	FString GetChampionName(const FSoftObjectPath& PawnData)
	{
		FString Name = PawnData.GetAssetName();
		Name.RemoveFromStart(TEXT("Data_"));
		return Name;
	}

	// InputTag.Ability1 -> Ability1.
	FString GetInputName(const FGameplayTag& InputTag)
	{
		const FString TagName = InputTag.ToString();
		int32 DotIndex = INDEX_NONE;
		return TagName.FindLastChar(TEXT('.'), DotIndex) ? TagName.RightChop(DotIndex + 1) : TagName;
	}

	struct FInputTestState
	{
		TUniquePtr<FAbilityTestRecorder> ClientRecorder;
		TUniquePtr<FAbilityTestRecorder> ServerRecorder;
		float EnemyHealthBefore = 0.f;
		float EnemyLowestHealth = 0.f;
		bool bCooldownSeen = false;
	};

	void AddInputSteps(FAbilityTestSteps& Steps, const FGameplayTag& InputTag, TSubclassOf<UGameplayAbility> AbilityClass)
	{
		using EResult = FAbilityTestSteps::EResult;

		FAbilityTestSession& Session = FAbilityTestSession::Get();
		FAutomationTestBase* Test = &Steps.GetTest();
		TSharedRef<FInputTestState> State = MakeShared<FInputTestState>();
		const UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>();

		// The cooldown and the enemy's health may change while the steps wait for other things, so every step after the
		// tap looks: whether the cooldown came on, and how low the enemy's health went(on the server).
		auto Observe = [&Session, State, AbilityCDO]
		{
			State->bCooldownSeen |= AbilityTestChecks::IsOnCooldown(Session.GetPlayers()[0].GetServerAbilitySystem(), AbilityCDO);
			State->EnemyLowestHealth = FMath::Min(State->EnemyLowestHealth, AbilityTestChecks::GetHealth(Session.GetPlayers()[1].GetServerAbilitySystem()));
		};

		Steps.Do(TEXT("Aim at the enemy champion"), [&Session, State, AbilityClass]
		{
			const FAbilityTestPlayer& Caster = Session.GetPlayers()[0];
			const FAbilityTestPlayer& Enemy = Session.GetPlayers()[1];
			FAbilityTestDriver::SetAim(Caster, FAbilityTestDriver::AimAt(Caster.FindClientCopyOf(Enemy)));

			State->ClientRecorder = MakeUnique<FAbilityTestRecorder>(Caster.GetClientAbilitySystem(), AbilityClass);
			State->ServerRecorder = MakeUnique<FAbilityTestRecorder>(Caster.GetServerAbilitySystem(), AbilityClass);
			State->EnemyHealthBefore = AbilityTestChecks::GetHealth(Enemy.GetServerAbilitySystem());
			State->EnemyLowestHealth = State->EnemyHealthBefore;
		});

		FAbilityTestDriver::AddTapSteps(Steps, 0, InputTag);

		Steps.Step(TEXT("Activates on the owning client"), 3.0, [State, Observe](FString& OutWaitReason)
		{
			Observe();
			OutWaitReason = State->ClientRecorder->GetFailures();
			return (State->ClientRecorder->GetNumActivations() > 0) ? EResult::Next : EResult::Wait;
		});

		Steps.Step(TEXT("Activates on the server"), 3.0, [State, Observe](FString& OutWaitReason)
		{
			Observe();
			OutWaitReason = State->ServerRecorder->GetFailures();
			return (State->ServerRecorder->GetNumActivations() > 0) ? EResult::Next : EResult::Wait;
		});

		// An attack keeps on until it is stopped: it has to hit first.
		if (InputTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("InputTag.Attack"))))
		{
			Steps.Step(TEXT("Hits the enemy champion"), 10.0, [&Session, State, Observe](FString& OutWaitReason)
			{
				Observe();
				const float Health = AbilityTestChecks::GetHealth(Session.GetPlayers()[1].GetServerAbilitySystem());
				OutWaitReason = FString::Printf(TEXT("the enemy's health is still %.0f"), Health);
				return (Health < State->EnemyHealthBefore) ? EResult::Next : EResult::Wait;
			});

			Steps.Do(TEXT("Stop attacking, as a move order would"), [&Session, AbilityClass]
			{
				UAssassinsAbilitySystemComponent* AbilitySystem = Session.GetPlayers()[0].GetClientAbilitySystem();
				if (AbilitySystem == nullptr)
				{
					return;
				}

				const FGameplayTagContainer MoveOrder(AssassinsGameplayTags::InputTag_SetDestination_Click);
				AbilitySystem->CancelAbilitiesWithCancelledByTag(&MoveOrder, nullptr);

				if (const FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(AbilityClass))
				{
					AbilitySystem->CancelAbilityHandle(Spec->Handle);
				}
			});
		}

		Steps.Step(TEXT("Ends on the owning client and the server"), 20.0, [State, Observe](FString& OutWaitReason)
		{
			Observe();
			const bool bClientActive = State->ClientRecorder->IsActive();
			const bool bServerActive = State->ServerRecorder->IsActive();
			if (!bClientActive && !bServerActive)
			{
				return EResult::Next;
			}

			OutWaitReason = FString::Printf(TEXT("still active on %s"), (bClientActive && bServerActive) ? TEXT("both") : (bClientActive ? TEXT("the owning client") : TEXT("the server")));
			return EResult::Wait;
		});

		// Damage that lands late(a projectile, a delayed burst) still counts.
		Steps.WaitFor(TEXT("Let the end replicate"), 0.5, Observe);

		// Reported, not judged: whether an ability should hurt the enemy standing 300 away depends on the ability. The
		// scenario tests judge it.
		Steps.Do(TEXT("Report the damage"), [Test, State, AbilityCDO]
		{
			const float Damage = State->EnemyHealthBefore - State->EnemyLowestHealth;
			const FString DamageEffects = AbilityTestChecks::FindDamageEffects(AbilityCDO);
			Test->AddInfo(FString::Printf(TEXT("Damage to the enemy: %.1f (health %.1f -> lowest %.1f). Damage effects: %s"),
				Damage, State->EnemyHealthBefore, State->EnemyLowestHealth, DamageEffects.IsEmpty() ? TEXT("none") : *DamageEffects));
			UE_LOG(LogAssassinsTests, Display, TEXT("Damage | %s | %.1f | %s"), *Test->GetTestFullName(), Damage, DamageEffects.IsEmpty() ? TEXT("none") : *DamageEffects);

			if (!DamageEffects.IsEmpty() && (Damage <= 0.f))
			{
				Test->AddWarning(TEXT("The ability has damage effects, but the enemy took no damage"));
			}
		});

		Steps.Step(TEXT("Puts its cooldown on"), FAbilityTestSteps::DefaultTimeoutSeconds, [Test, State, AbilityCDO](FString& OutError)
		{
			if (!AbilityTestChecks::HasCooldownEffect(AbilityCDO))
			{
				Test->AddInfo(TEXT("Cooldown check skipped: the ability has no cooldown effect with tags"));
				return EResult::Next;
			}

			if (State->bCooldownSeen)
			{
				return EResult::Next;
			}

			OutError = TEXT("the ability has a cooldown effect, but its tags never came on the server");
			return EResult::Fail;
		});

		// Some abilities go on attacking on purpose(Zed's R strikes, then attacks its target). A running attack holds tags of
		// its own, which are not left over: it is stopped first, and its end has to clear them.
		Steps.Do(TEXT("Stop the attacks the ability went on with"), [&Session]
		{
			AbilityScenarios::StopAttacking(Session.GetPlayers()[0]);
		});

		Steps.Step(TEXT("The attacks end on the owning client and the server"), 5.0, [&Session](FString& OutWaitReason)
		{
			const FAbilityTestPlayer& Caster = Session.GetPlayers()[0];
			if (!AbilityScenarios::IsAttacking(Caster.GetClientAbilitySystem()) && !AbilityScenarios::IsAttacking(Caster.GetServerAbilitySystem()))
			{
				return EResult::Next;
			}

			OutWaitReason = TEXT("an attack still runs");
			return EResult::Wait;
		});

		Steps.Step(TEXT("Leaves no transient tags"), FAbilityTestSteps::DefaultTimeoutSeconds, [&Session](FString& OutError)
		{
			const FAbilityTestPlayer& Caster = Session.GetPlayers()[0];
			const FString OnClient = AbilityTestChecks::FindLeftoverTags(Caster.GetClientAbilitySystem());
			const FString OnServer = AbilityTestChecks::FindLeftoverTags(Caster.GetServerAbilitySystem());
			if (OnClient.IsEmpty() && OnServer.IsEmpty())
			{
				return EResult::Next;
			}

			OutError = FString::Printf(TEXT("owning client [%s], server [%s]"), *OnClient, *OnServer);
			return EResult::Fail;
		});

		Steps.Finally([&Session, State]
		{
			if (Session.GetPlayers().Num() > 0)
			{
				FAbilityTestDriver::ClearAim(Session.GetPlayers()[0]);
			}
			State->ClientRecorder.Reset();
			State->ServerRecorder.Reset();
		});
	}

	void AddPassiveSteps(FAbilityTestSteps& Steps, TSubclassOf<UGameplayAbility> AbilityClass)
	{
		FAbilityTestSession& Session = FAbilityTestSession::Get();

		// It starts as the champion spawns and keeps on.
		Steps.WaitUntil(TEXT("Is active on the server"), 5.0, [&Session, AbilityClass]
		{
			const UAbilitySystemComponent* AbilitySystem = Session.GetPlayers()[0].GetServerAbilitySystem();
			const FGameplayAbilitySpec* Spec = AbilitySystem ? AbilitySystem->FindAbilitySpecFromClass(AbilityClass) : nullptr;
			return Spec && Spec->IsActive();
		});
	}
}

static void GetSmokeTestList(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands)
{
	const TArray<FSoftObjectPath> Champions = AbilitySmokeTests::FindChampions();
	for (int32 Index = 0; Index < Champions.Num(); ++Index)
	{
		const FSoftObjectPath& Champion = Champions[Index];
		const FSoftObjectPath& Enemy = Champions[(Index + 1) % Champions.Num()];
		const UAssassinsPawnData* PawnData = Cast<UAssassinsPawnData>(Champion.TryLoad());
		if (PawnData == nullptr)
		{
			continue;
		}

		const FString ChampionName = AbilitySmokeTests::GetChampionName(Champion);

		// Leblanc's R does what her last ability did, which a session keeps between tests: from a new one, that's Q.
		bool bMimics = false;
		for (const UAssassinsAbilitySet* AbilitySet : PawnData->AbilitySets)
		{
			bMimics |= AbilitySet && AbilitySet->GetSkillStateClass() && AbilitySet->GetSkillStateClass()->IsChildOf(UAssassinsChampionSkillState_Leblanc::StaticClass());
		}

		for (const UAssassinsAbilitySet* AbilitySet : PawnData->AbilitySets)
		{
			if (AbilitySet == nullptr)
			{
				continue;
			}

			for (const FAssassinsAbilitySet_GameplayAbility& Granted : AbilitySet->GetGrantedGameplayAbilities())
			{
				const UAssassinsGameplayAbility* AbilityCDO = Granted.Ability ? Granted.Ability->GetDefaultObject<UAssassinsGameplayAbility>() : nullptr;
				if (AbilityCDO == nullptr)
				{
					continue;
				}

				FString What;
				FString BeautifiedName;
				if (Granted.InputTag.IsValid())
				{
					What = Granted.InputTag.ToString();
					BeautifiedName = FString::Printf(TEXT("%s.%s"), *ChampionName, *AbilitySmokeTests::GetInputName(Granted.InputTag));
				}
				else if (AbilityCDO->GetActivationPolicy() == EAssassinsAbilityActivationPolicy::OnSpawn)
				{
					What = AbilitySmokeTests::PassiveTest;
					BeautifiedName = FString::Printf(TEXT("%s.Passive.%s"), *ChampionName, *Granted.Ability->GetName().LeftChop(2));
				}
				else
				{
					continue;
				}

				if (OutBeautifiedNames.Contains(BeautifiedName))
				{
					BeautifiedName += FString::Printf(TEXT(".%s"), *Granted.Ability->GetName().LeftChop(2));
				}

				const bool bFresh = bMimics && Granted.InputTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("InputTag.Ability4")));

				OutBeautifiedNames.Add(BeautifiedName);
				OutTestCommands.Add(FString::Join(TArray<FString>{ Champion.ToString(), Enemy.ToString(), What, Granted.Ability->GetPathName(), bFresh ? TEXT("Fresh") : TEXT("") }, TEXT(";")));
			}
		}
	}
}

static bool RunSmokeTest(FAutomationTestBase& Test, const FString& Parameters, bool bListenServer, int32 LagMs)
{
	TArray<FString> Arguments;
	Parameters.ParseIntoArray(Arguments, TEXT(";"));
	if (Arguments.Num() < 4)
	{
		Test.AddError(FString::Printf(TEXT("Bad test parameters: %s"), *Parameters));
		return false;
	}

	UClass* AbilityClass = LoadClass<UGameplayAbility>(nullptr, *Arguments[3]);
	if (AbilityClass == nullptr)
	{
		Test.AddError(FString::Printf(TEXT("No ability class %s"), *Arguments[3]));
		return false;
	}

	FAbilityTestSessionConfig Config;
	Config.Champions = { FSoftObjectPath(Arguments[0]), FSoftObjectPath(Arguments[1]) };
	Config.bListenServer = bListenServer;
	Config.LagMs = LagMs;
	Config.bFreshSession = (Arguments.Num() > 4) && (Arguments[4] == TEXT("Fresh"));

	TSharedRef<FAbilityTestSteps> Steps = MakeShared<FAbilityTestSteps>(Test);
	FAbilityTestSession::Get().AddStartSteps(*Steps, Config);
	FAbilityTestSession::Get().AddResetSteps(*Steps);

	if (Arguments[2] == AbilitySmokeTests::PassiveTest)
	{
		AbilitySmokeTests::AddPassiveSteps(*Steps, AbilityClass);
	}
	else
	{
		AbilitySmokeTests::AddInputSteps(*Steps, FGameplayTag::RequestGameplayTag(FName(*Arguments[2])), AbilityClass);
	}

	Steps->Run();
	return true;
}

void FAssassinsAbilitySmokeTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetSmokeTestList(OutBeautifiedNames, OutTestCommands);
}

bool FAssassinsAbilitySmokeTest::RunTest(const FString& Parameters)
{
	return RunSmokeTest(*this, Parameters, /*bListenServer*/ false, /*LagMs*/ 0);
}

#endif // WITH_DEV_AUTOMATION_TESTS
