// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityScenario.h"
#include "AssassinsTestsLog.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace AbilityScenarios
{
	const TCHAR* Zed = TEXT("/Game/Champions/Blueprints/Zed/Data_Zed.Data_Zed");
	const TCHAR* Akali = TEXT("/Game/Champions/Blueprints/Akali/Data_Akali.Data_Akali");
	const TCHAR* Leblanc = TEXT("/Game/Champions/Blueprints/Leblanc/Data_Leblanc.Data_Leblanc");

	// The inputs a test may leave pressed when it fails midway.
	const TCHAR* const AbilityInputs[] = { TEXT("InputTag.Ability1"), TEXT("InputTag.Ability2"), TEXT("InputTag.Ability3"), TEXT("InputTag.Ability4"), TEXT("InputTag.Attack") };

	FGameplayTag Input(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}

	const FAbilityTestPlayer& PlayerAt(int32 Index)
	{
		return FAbilityTestSession::Get().GetPlayers()[Index];
	}

	const FAbilityTestPlayer& Caster()
	{
		return PlayerAt(0);
	}

	const FAbilityTestPlayer& Enemy()
	{
		return PlayerAt(1);
	}

	float EnemyHealth()
	{
		return AbilityTestChecks::GetHealth(Enemy().GetServerAbilitySystem());
	}

	bool HasTag(const UAbilitySystemComponent* AbilitySystem, const FGameplayTag& Tag)
	{
		return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(Tag);
	}

	void Observe(FState& State)
	{
		const FAbilityTestSession& Session = FAbilityTestSession::Get();
		if (Session.GetPlayers().Num() < 2)
		{
			return;
		}

		State.EnemyLowestHealth = FMath::Min(State.EnemyLowestHealth, EnemyHealth());
		State.MostProjectiles = FMath::Max(State.MostProjectiles, CountActors<AAssassinsProjectile>(Session.GetServerWorld()));
		State.bEnemyRootedSeen |= HasTag(Enemy().GetServerAbilitySystem(), AssassinsGameplayTags::Status_Rooted);
	}

	UClass* FindAbilityClass(const UAbilitySystemComponent* AbilitySystem, const TCHAR* ClassNamePart)
	{
		if (AbilitySystem)
		{
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.Ability->GetClass()->GetName().Contains(ClassNamePart))
				{
					return Spec.Ability->GetClass();
				}
			}
		}
		return nullptr;
	}

	UClass* FindAbilityClass(const UAbilitySystemComponent* AbilitySystem, const FGameplayTag& InputTag)
	{
		if (AbilitySystem)
		{
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
				{
					return Spec.Ability->GetClass();
				}
			}
		}
		return nullptr;
	}

	FString FindActiveAbility(const UAbilitySystemComponent* AbilitySystem)
	{
		if (AbilitySystem)
		{
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				const UAssassinsGameplayAbility* AbilityCDO = Cast<UAssassinsGameplayAbility>(Spec.Ability);
				const bool bPassive = AbilityCDO && (AbilityCDO->GetActivationPolicy() == EAssassinsAbilityActivationPolicy::OnSpawn);
				if (Spec.IsActive() && !bPassive)
				{
					return GetNameSafe(Spec.Ability);
				}
			}
		}
		return FString();
	}

	bool IsCasterNear(const FVector& Location, double Tolerance)
	{
		const AActor* Champion = Caster().ServerChampion.Get();
		return Champion && (FVector::Dist2D(Champion->GetActorLocation(), Location) <= Tolerance);
	}

	bool IsCasterNearEnemy(double Tolerance)
	{
		const AActor* Target = Enemy().ServerChampion.Get();
		return Target && IsCasterNear(Target->GetActorLocation(), Tolerance);
	}

	double GetDistancePast(const AActor* Champion, const AActor* Target)
	{
		if ((Champion == nullptr) || (Target == nullptr))
		{
			return 0.0;
		}

		const FVector Away = (Target->GetActorLocation() - FAbilityTestSession::GetStartLocation(0)).GetSafeNormal2D();
		const FVector FromTarget = Champion->GetActorLocation() - Target->GetActorLocation();
		return FVector::DotProduct(FVector(FromTarget.X, FromTarget.Y, 0.0), Away);
	}

	void Place(AAssassinsChampion* Champion, const FVector& FloorLocation)
	{
		if (Champion)
		{
			Champion->GetCharacterMovement()->StopMovementImmediately();
			const float HalfHeight = Champion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			Champion->TeleportTo(FloorLocation + FVector(0.0, 0.0, HalfHeight + 2.0), Champion->GetActorRotation(), /*bIsATest*/ false, /*bNoCheck*/ true);
		}
	}

	void MovePlayer(FAbilityTestSteps& Steps, int32 PlayerIndex, const FVector& FloorLocation)
	{
		Steps.Do(FString::Printf(TEXT("Put player %d at %s"), PlayerIndex, *FloorLocation.ToCompactString()), [PlayerIndex, FloorLocation]
		{
			Place(PlayerAt(PlayerIndex).ServerChampion.Get(), FloorLocation);
			Place(PlayerAt(PlayerIndex).ClientChampion.Get(), FloorLocation);
		});

		if (PlayerIndex != 0)
		{
			WaitUntilNear(Steps, TEXT("The caster sees it there"), 3.0, [PlayerIndex] { return Caster().FindClientCopyOf(PlayerAt(PlayerIndex)); }, [FloorLocation] { return FloorLocation; }, 30.0);
		}
	}

	FAbilityTestSessionConfig Versus(const TCHAR* CasterChampion, const TCHAR* EnemyChampion, bool bFreshSession)
	{
		FAbilityTestSessionConfig Config;
		Config.Champions = { FSoftObjectPath(CasterChampion), FSoftObjectPath(EnemyChampion) };
		Config.bFreshSession = bFreshSession;
		return Config;
	}

	bool Run(FAutomationTestBase& Test, const FAbilityTestSessionConfig& Config, FAddSteps AddSteps)
	{
		TSharedRef<FAbilityTestSteps> Steps = MakeShared<FAbilityTestSteps>(Test);
		TSharedRef<FState> State = MakeShared<FState>();

		FAbilityTestSession::Get().AddStartSteps(*Steps, Config);
		FAbilityTestSession::Get().AddResetSteps(*Steps);
		Steps->Do(TEXT("Note the enemy's health"), [State]
		{
			State->EnemyHealthBefore = EnemyHealth();
			State->EnemyLowestHealth = State->EnemyHealthBefore;
			State->DamageLog = MakeUnique<FAbilityTestDamageLog>(Enemy().GetServerAbilitySystem());
		});
		Steps->EachFrame([State] { Observe(*State); });

		AddSteps(*Steps, State);

		FAutomationTestBase* TestPtr = &Test;
		Steps->Finally([TestPtr, State]
		{
			if (FAbilityTestSession::Get().GetPlayers().Num() > 0)
			{
				for (const TCHAR* InputName : AbilityInputs)
				{
					FAbilityTestDriver::ReleaseInput(Caster(), Input(InputName));
				}
				FAbilityTestDriver::ClearAim(Caster());
			}

			if (State->DamageLog)
			{
				TestPtr->AddInfo(FString::Printf(TEXT("Damage to the enemy by ability: %s"), *State->DamageLog->DescribeByAbility()));
				UE_LOG(LogAssassinsTests, Display, TEXT("Damage log | %s\n%s"), *TestPtr->GetTestFullName(), *State->DamageLog->Describe());
			}
			State->Recorder.Reset();
			State->DamageLog.Reset();
		});
		Steps->Run();
		return true;
	}

	struct FRegisteredScenario
	{
		FString Name;
		TFunction<FAbilityTestSessionConfig()> GetConfig;
		FAddSteps AddSteps;
		bool bWithVariants = true;
	};

	// By group, in the order the files register them. Made on first use: the registrations are statics of other files.
	static TMap<FString, TArray<FRegisteredScenario>>& GetScenarioRegistry()
	{
		static TMap<FString, TArray<FRegisteredScenario>> Registry;
		return Registry;
	}

	FScenarioRegistration::FScenarioRegistration(const TCHAR* Group, const TCHAR* Name, const TCHAR* CasterChampion, const TCHAR* EnemyChampion, FAddSteps AddSteps)
		: FScenarioRegistration(Group, Name, CasterChampion, EnemyChampion, /*bWithVariants*/ true, MoveTemp(AddSteps))
	{
	}

	FScenarioRegistration::FScenarioRegistration(const TCHAR* Group, const TCHAR* Name, const TCHAR* CasterChampion, const TCHAR* EnemyChampion, bool bWithVariants, FAddSteps AddSteps)
	{
		GetScenarioRegistry().FindOrAdd(Group).Add({ Name, [CasterChampion, EnemyChampion] { return Versus(CasterChampion, EnemyChampion); }, MoveTemp(AddSteps), bWithVariants });
	}

	FScenarioRegistration::FScenarioRegistration(const TCHAR* Group, const TCHAR* Name, TFunction<FAbilityTestSessionConfig()> GetConfig, FAddSteps AddSteps)
	{
		GetScenarioRegistry().FindOrAdd(Group).Add({ Name, MoveTemp(GetConfig), MoveTemp(AddSteps) });
	}

	void GetScenarioTests(const TCHAR* Group, TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands, bool bVariant)
	{
		if (const TArray<FRegisteredScenario>* Scenarios = GetScenarioRegistry().Find(Group))
		{
			for (const FRegisteredScenario& Scenario : *Scenarios)
			{
				if (bVariant && !Scenario.bWithVariants)
				{
					continue;
				}

				OutBeautifiedNames.Add(Scenario.Name);
				OutTestCommands.Add(Scenario.Name);
			}
		}
	}

	bool RunScenarioTest(FAutomationTestBase& Test, const TCHAR* Group, const FString& Name, bool bListenServer, int32 LagMs)
	{
		const TArray<FRegisteredScenario>* Scenarios = GetScenarioRegistry().Find(Group);
		const FRegisteredScenario* Scenario = Scenarios ? Scenarios->FindByPredicate([&Name](const FRegisteredScenario& Each) { return Each.Name == Name; }) : nullptr;
		if (Scenario == nullptr)
		{
			Test.AddError(FString::Printf(TEXT("No scenario %s in the group %s"), *Name, Group));
			return false;
		}

		FAbilityTestSessionConfig Config = Scenario->GetConfig();
		Config.bListenServer = bListenServer;
		Config.LagMs = LagMs;
		return Run(Test, Config, Scenario->AddSteps);
	}

	FVector SpotBesideCaster(double Distance)
	{
		return FAbilityTestSession::GetStartLocation(0) + FVector(Distance, 0.0, 0.0);
	}

	FVector SpotBesideEnemy(double Distance)
	{
		return FAbilityTestSession::GetStartLocation(1) + FVector(Distance, 0.0, 0.0);
	}

	FVector SpotTowardEnemy(double Distance)
	{
		const FVector Start = FAbilityTestSession::GetStartLocation(0);
		return Start + (FAbilityTestSession::GetStartLocation(1) - Start).GetSafeNormal2D() * Distance;
	}

	FGetAim AimAtEnemy()
	{
		return AimAtPlayer(1);
	}

	FGetAim AimAtPlayer(int32 PlayerIndex)
	{
		return [PlayerIndex]
		{
			return FAbilityTestDriver::AimAt(Caster().FindClientCopyOf(PlayerAt(PlayerIndex)));
		};
	}

	FGetAim AimAtSpot(const FVector& Spot)
	{
		return [Spot]
		{
			FAbilityTestAim Aim;
			Aim.Location = Spot;
			return Aim;
		};
	}

	void TapAtEnemy(FAbilityTestSteps& Steps, const FGameplayTag& InputTag)
	{
		Steps.Do(FString::Printf(TEXT("Aim at the enemy for %s"), *InputTag.ToString()), []
		{
			FAbilityTestDriver::SetAim(Caster(), FAbilityTestDriver::AimAt(Caster().FindClientCopyOf(Enemy())));
		});
		FAbilityTestDriver::AddTapSteps(Steps, 0, InputTag);
	}

	void TapAtSpot(FAbilityTestSteps& Steps, const FGameplayTag& InputTag, const FVector& Spot)
	{
		Steps.Do(FString::Printf(TEXT("Aim at %s for %s"), *Spot.ToCompactString(), *InputTag.ToString()), [Spot]
		{
			FAbilityTestAim Aim;
			Aim.Location = Spot;
			FAbilityTestDriver::SetAim(Caster(), Aim);
		});
		FAbilityTestDriver::AddTapSteps(Steps, 0, InputTag);
	}

	void WaitForDamage(FAbilityTestSteps& Steps, TSharedRef<FState> State, double TimeoutSeconds)
	{
		Steps.Step(TEXT("The enemy takes damage"), TimeoutSeconds, [State](FString& OutWaitReason)
		{
			Observe(*State);
			OutWaitReason = FString::Printf(TEXT("the enemy's health is still %.1f"), EnemyHealth());
			return (State->EnemyLowestHealth < State->EnemyHealthBefore) ? EResult::Next : EResult::Wait;
		});
	}

	void CheckDamage(FAbilityTestSteps& Steps, TSharedRef<FState> State, double WatchSeconds, float Expected)
	{
		FAutomationTestBase* Test = &Steps.GetTest();
		Steps.WaitFor(TEXT("Let the damage land"), WatchSeconds, [State] { Observe(*State); });
		Steps.Step(TEXT("Deals the damage"), FAbilityTestSteps::DefaultTimeoutSeconds, [Test, State, Expected](FString& OutError)
		{
			const float Damage = State->EnemyHealthBefore - State->EnemyLowestHealth;
			UE_LOG(LogAssassinsTests, Display, TEXT("Damage | %s | %.1f | expected %s"), *Test->GetTestFullName(), Damage, (Expected < 0.f) ? TEXT("any") : *FString::SanitizeFloat(Expected));

			const bool bDealt = (Expected < 0.f) ? (Damage > 0.f) : (FMath::Abs(Damage - Expected) <= Expected * DamageTolerance);
			if (bDealt)
			{
				return EResult::Next;
			}

			OutError = (Expected < 0.f)
				? TEXT("expected damage, dealt none")
				: FString::Printf(TEXT("expected damage %.1f, dealt %.1f"), Expected, Damage);
			return EResult::Fail;
		});
	}

	static void CheckDamageFromClass(FAbilityTestSteps& Steps, TSharedRef<FState> State, const FString& Name, TFunction<UClass*()> GetAbilityClass, float Expected)
	{
		FAutomationTestBase* Test = &Steps.GetTest();
		Steps.Step(FString::Printf(TEXT("%s deals its damage"), *Name), FAbilityTestSteps::DefaultTimeoutSeconds, [Test, State, GetAbilityClass, Expected](FString& OutError)
		{
			const FString ClassName = GetNameSafe(GetAbilityClass());
			const float Damage = State->DamageLog ? State->DamageLog->GetDamage(ClassName) : 0.f;
			UE_LOG(LogAssassinsTests, Display, TEXT("Damage | %s | %s | %.1f | expected %s"), *Test->GetTestFullName(), *ClassName, Damage, (Expected < 0.f) ? TEXT("any") : *FString::SanitizeFloat(Expected));

			const bool bDealt = (Expected < 0.f) ? (Damage > 0.f) : (FMath::Abs(Damage - Expected) <= Expected * DamageTolerance);
			if (bDealt)
			{
				return EResult::Next;
			}

			OutError = (Expected < 0.f)
				? FString::Printf(TEXT("%s dealt no damage"), *ClassName)
				: FString::Printf(TEXT("%s: expected damage %.1f, dealt %.1f"), *ClassName, Expected, Damage);
			return EResult::Fail;
		});
	}

	void CheckDamageFrom(FAbilityTestSteps& Steps, TSharedRef<FState> State, const FGameplayTag& InputTag, float Expected)
	{
		CheckDamageFromClass(Steps, State, InputTag.ToString(), [InputTag] { return FindAbilityClass(Caster().GetServerAbilitySystem(), InputTag); }, Expected);
	}

	void CheckDamageFrom(FAbilityTestSteps& Steps, TSharedRef<FState> State, const TCHAR* ClassNamePart, float Expected)
	{
		const FString Part = ClassNamePart;
		CheckDamageFromClass(Steps, State, Part, [Part] { return FindAbilityClass(Caster().GetServerAbilitySystem(), *Part); }, Expected);
	}

	void WaitForEffectFrom(FAbilityTestSteps& Steps, TSharedRef<FState> State, const FString& Description, const FGameplayTag& InputTag, const TCHAR* EffectNamePart, double TimeoutSeconds)
	{
		const FString Part = EffectNamePart;
		Steps.Step(Description, TimeoutSeconds, [State, InputTag, Part](FString& OutWaitReason)
		{
			const FString Ability = GetNameSafe(FindAbilityClass(Caster().GetServerAbilitySystem(), InputTag));
			const bool bApplied = State->DamageLog && State->DamageLog->GetEntries().ContainsByPredicate([&Ability, &Part](const FAbilityTestDamageLog::FEntry& Entry)
			{
				return (Entry.Ability == Ability) && Entry.Effect.Contains(Part);
			});
			if (bApplied)
			{
				return EResult::Next;
			}

			OutWaitReason = FString::Printf(TEXT("%s has put no effect named ...%s... on the enemy"), *Ability, *Part);
			return EResult::Wait;
		});
	}

	void WaitUntilNear(FAbilityTestSteps& Steps, const FString& Description, double TimeoutSeconds, TFunction<const AActor*()> GetActor, TFunction<FVector()> GetLocation, double Tolerance)
	{
		Steps.Step(Description, TimeoutSeconds, [GetActor, GetLocation, Tolerance](FString& OutWaitReason)
		{
			const AActor* Actor = GetActor();
			const double Distance = Actor ? FVector::Dist2D(Actor->GetActorLocation(), GetLocation()) : TNumericLimits<double>::Max();
			OutWaitReason = FString::Printf(TEXT("%.0f away"), Distance);
			return (Distance <= Tolerance) ? EResult::Next : EResult::Wait;
		});
	}

	void WaitForTag(FAbilityTestSteps& Steps, const FString& Description, double TimeoutSeconds, TFunction<const UAbilitySystemComponent*()> GetAbilitySystem, const FGameplayTag& Tag, bool bPresent)
	{
		Steps.WaitUntil(Description, TimeoutSeconds, [GetAbilitySystem, Tag, bPresent]
		{
			return HasTag(GetAbilitySystem(), Tag) == bPresent;
		});
	}

	void TapUntil(FAbilityTestSteps& Steps, const FString& Description, const FGameplayTag& InputTag, FGetAim GetAim, double TimeoutSeconds, TFunction<bool()> Condition)
	{
		TSharedRef<double> LastTapTime = MakeShared<double>(0.0);
		TSharedRef<bool> bPressed = MakeShared<bool>(false);
		Steps.Step(Description, TimeoutSeconds, [InputTag, GetAim, Condition, LastTapTime, bPressed](FString& OutWaitReason)
		{
			if (*bPressed)
			{
				FAbilityTestDriver::ReleaseInput(Caster(), InputTag);
				*bPressed = false;
			}

			if (Condition())
			{
				return EResult::Next;
			}

			const double Now = FPlatformTime::Seconds();
			if (Now - *LastTapTime >= RetapSeconds)
			{
				FAbilityTestDriver::SetAim(Caster(), GetAim());
				FAbilityTestDriver::PressInput(Caster(), InputTag);
				*bPressed = true;
				*LastTapTime = Now;
			}

			OutWaitReason = TEXT("still waiting after tapping");
			return EResult::Wait;
		});
	}

	void TapAtEnemyUntil(FAbilityTestSteps& Steps, const FString& Description, const FGameplayTag& InputTag, double TimeoutSeconds, TFunction<bool()> Condition)
	{
		TapUntil(Steps, Description, InputTag, AimAtEnemy(), TimeoutSeconds, MoveTemp(Condition));
	}

	void CastAbility(FAbilityTestSteps& Steps, const FString& Name, const FGameplayTag& InputTag, FGetAim GetAim, const TCHAR* ExpectedAbility, double TimeoutSeconds)
	{
		struct FCasting
		{
			// The ability the input starts, and the one that has to start(the same one unless ExpectedAbility names
			// another), on the owning client; the one that has to start on the server.
			TUniquePtr<FAbilityTestRecorder> InputOnClient;
			TUniquePtr<FAbilityTestRecorder> ExpectedOnClient;
			TUniquePtr<FAbilityTestRecorder> ExpectedOnServer;
			FString ExpectedName;
			double LastTapTime = 0.0;
			bool bPressed = false;
		};
		TSharedRef<FCasting> Casting = MakeShared<FCasting>();
		const FString Expected = ExpectedAbility ? FString(ExpectedAbility) : FString();

		Steps.Do(FString::Printf(TEXT("%s: watch for it"), *Name), [Casting, InputTag, Expected]
		{
			UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
			UAbilitySystemComponent* ServerAbilitySystem = Caster().GetServerAbilitySystem();
			UClass* InputClass = FindAbilityClass(ClientAbilitySystem, InputTag);
			UClass* ExpectedClass = Expected.IsEmpty() ? InputClass : FindAbilityClass(ClientAbilitySystem, *Expected);
			Casting->InputOnClient = MakeUnique<FAbilityTestRecorder>(ClientAbilitySystem, InputClass);
			Casting->ExpectedOnClient = MakeUnique<FAbilityTestRecorder>(ClientAbilitySystem, ExpectedClass);
			Casting->ExpectedOnServer = MakeUnique<FAbilityTestRecorder>(ServerAbilitySystem, ExpectedClass);
			Casting->ExpectedName = GetNameSafe(ExpectedClass);
		});

		Steps.Step(FString::Printf(TEXT("%s: starts on the owning client"), *Name), TimeoutSeconds, [Casting, InputTag, GetAim](FString& OutWaitReason)
		{
			if (Casting->bPressed)
			{
				FAbilityTestDriver::ReleaseInput(Caster(), InputTag);
				Casting->bPressed = false;
			}

			if (Casting->ExpectedOnClient->GetNumActivations() > 0)
			{
				return EResult::Next;
			}

			// A player presses once the cast before is over. Pressed during it, an ability that the cast does not block
			// cuts it short(Leblanc's W stops Q before the throw), or starts without what it needs(Leblanc's R ends at
			// once when the mimic can't start).
			if (HasTag(Caster().GetClientAbilitySystem(), AssassinsGameplayTags::Status_Channeling))
			{
				OutWaitReason = TEXT("the cast before is still going");
				return EResult::Wait;
			}

			// While the input's ability runs a press would be its recast: wait for it to start the expected one or end.
			if (Casting->InputOnClient->IsActive())
			{
				OutWaitReason = FString::Printf(TEXT("waiting for %s"), *Casting->ExpectedName);
				return EResult::Wait;
			}

			const double Now = FPlatformTime::Seconds();
			if (Now - Casting->LastTapTime >= RetapSeconds)
			{
				FAbilityTestDriver::SetAim(Caster(), GetAim());
				FAbilityTestDriver::PressInput(Caster(), InputTag);
				Casting->bPressed = true;
				Casting->LastTapTime = Now;
			}

			const FString& Failures = Casting->ExpectedOnClient->GetFailures().IsEmpty() ? Casting->InputOnClient->GetFailures() : Casting->ExpectedOnClient->GetFailures();
			OutWaitReason = Failures.IsEmpty() ? FString(TEXT("pressed, not started yet")) : Failures;
			return EResult::Wait;
		});

		Steps.Step(FString::Printf(TEXT("%s: activates on the server"), *Name), TimeoutSeconds, [Casting](FString& OutWaitReason)
		{
			if (Casting->ExpectedOnServer->GetNumActivations() > 0)
			{
				return EResult::Next;
			}

			OutWaitReason = FString::Printf(TEXT("%s did not start on the server%s%s"), *Casting->ExpectedName,
				Casting->ExpectedOnServer->GetFailures().IsEmpty() ? TEXT("") : TEXT(": "), *Casting->ExpectedOnServer->GetFailures());
			return EResult::Wait;
		});
	}

	// The attack, and the ability that keeps attacking between two.
	static const FGameplayTagContainer& GetAttackTags()
	{
		static const FGameplayTagContainer Tags = []
		{
			FGameplayTagContainer Result;
			Result.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack")));
			Result.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.ActivateAttack")));
			return Result;
		}();
		return Tags;
	}

	bool IsAttacking(const UAbilitySystemComponent* AbilitySystem)
	{
		if (AbilitySystem)
		{
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				if (Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasAny(GetAttackTags()))
				{
					return true;
				}
			}
		}
		return false;
	}

	void AttackOnce(FAbilityTestSteps& Steps, TSharedRef<FState> State, double TimeoutSeconds)
	{
		struct FAttack
		{
			FString AttackClassName;
			double StartTime = 0.0;
			double LastTapTime = 0.0;
			bool bPressed = false;
		};
		TSharedRef<FAttack> Attack = MakeShared<FAttack>();
		const FGameplayTag AttackInput = Input(TEXT("InputTag.Attack"));

		Steps.Do(TEXT("Attack: watch for it"), [Attack, AttackInput]
		{
			Attack->AttackClassName = GetNameSafe(FindAbilityClass(Caster().GetClientAbilitySystem(), AttackInput));
			Attack->StartTime = FPlatformTime::Seconds();
		});

		// Pressed once the cast or the dash before is over, and again whenever no attack goes on without having hit(the
		// server may turn down an attack the client started a moment early). An attack going on is left alone: a press
		// would start it over.
		Steps.Step(TEXT("Attack: hits the enemy"), TimeoutSeconds, [Attack, AttackInput, State](FString& OutWaitReason)
		{
			if (Attack->bPressed)
			{
				FAbilityTestDriver::ReleaseInput(Caster(), AttackInput);
				Attack->bPressed = false;
			}

			if (State->DamageLog && (State->DamageLog->CountHits(Attack->AttackClassName, Attack->StartTime) > 0))
			{
				return EResult::Next;
			}

			const UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
			if (HasTag(ClientAbilitySystem, AssassinsGameplayTags::Status_Channeling) || HasTag(ClientAbilitySystem, AssassinsGameplayTags::Status_Dashing))
			{
				OutWaitReason = TEXT("the cast or the dash before is still going");
				return EResult::Wait;
			}

			const double Now = FPlatformTime::Seconds();
			if (!IsAttacking(Caster().GetClientAbilitySystem()) && (Now - Attack->LastTapTime >= RetapSeconds))
			{
				FAbilityTestDriver::SetAim(Caster(), FAbilityTestDriver::AimAt(Caster().FindClientCopyOf(Enemy())));
				FAbilityTestDriver::PressInput(Caster(), AttackInput);
				Attack->bPressed = true;
				Attack->LastTapTime = Now;
			}

			OutWaitReason = FString::Printf(TEXT("no hit from %s yet"), *Attack->AttackClassName);
			return EResult::Wait;
		});
	}

	void StopAttacking(const FAbilityTestPlayer& StoppingPlayer)
	{
		UAbilitySystemComponent* AbilitySystem = StoppingPlayer.GetClientAbilitySystem();
		if (AbilitySystem == nullptr)
		{
			return;
		}

		TArray<FGameplayAbilitySpecHandle> ToCancel;
		for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
		{
			if (Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasAny(GetAttackTags()))
			{
				ToCancel.Add(Spec.Handle);
			}
		}

		for (const FGameplayAbilitySpecHandle& Handle : ToCancel)
		{
			AbilitySystem->CancelAbilityHandle(Handle);
		}
	}

	void ThrowShurikens(FAbilityTestSteps& Steps, TSharedRef<FState> State, int32 AtLeast)
	{
		Steps.Do(TEXT("Count the shurikens from now"), [State] { State->MostProjectiles = 0; });
		CastAbility(Steps, TEXT("Q"), Input(TEXT("InputTag.Ability1")), AimAtEnemy());
		Steps.WaitFor(TEXT("Let the shurikens fly"), 1.5);
		Steps.Step(FString::Printf(TEXT("%d shurikens fly at once on the server"), AtLeast), FAbilityTestSteps::DefaultTimeoutSeconds, [State, AtLeast](FString& OutError)
		{
			OutError = FString::Printf(TEXT("at most %d at once"), State->MostProjectiles);
			return (State->MostProjectiles >= AtLeast) ? EResult::Next : EResult::Fail;
		});
	}

	void WaitUntilBehindEnemy(FAbilityTestSteps& Steps, TSharedRef<double> OutArrivalTime)
	{
		Steps.Step(TEXT("Zed reappears behind the enemy"), 5.0, [OutArrivalTime](FString& OutWaitReason)
		{
			const AActor* Champion = Caster().ServerChampion.Get();
			const AActor* Target = Enemy().ServerChampion.Get();
			if ((Champion == nullptr) || (Target == nullptr))
			{
				OutWaitReason = TEXT("a champion is missing on the server");
				return EResult::Wait;
			}

			const double Past = GetDistancePast(Champion, Target);
			const double Distance = FVector::Dist2D(Champion->GetActorLocation(), Target->GetActorLocation());
			if ((Past > 50.0) && (Distance < 250.0))
			{
				*OutArrivalTime = FPlatformTime::Seconds();
				return EResult::Next;
			}

			OutWaitReason = FString::Printf(TEXT("%.0f from the enemy, %.0f past it"), Distance, Past);
			return EResult::Wait;
		});
	}

	void FinishCombo(FAbilityTestSteps& Steps, double TimeoutSeconds)
	{
		Steps.Do(TEXT("Stop attacking"), [] { StopAttacking(Caster()); });

		Steps.Step(TEXT("Every ability ends on the owning client and the server"), TimeoutSeconds, [](FString& OutWaitReason)
		{
			const FString OnServer = FindActiveAbility(Caster().GetServerAbilitySystem());
			const FString OnClient = FindActiveAbility(Caster().GetClientAbilitySystem());
			if (OnServer.IsEmpty() && OnClient.IsEmpty())
			{
				return EResult::Next;
			}

			OutWaitReason = FString::Printf(TEXT("still running: %s on the server, %s on the owning client"),
				OnServer.IsEmpty() ? TEXT("nothing") : *OnServer, OnClient.IsEmpty() ? TEXT("nothing") : *OnClient);
			return EResult::Wait;
		});

		Steps.WaitFor(TEXT("Let the end replicate"), 0.5);

		Steps.Step(TEXT("Leaves no transient tags"), FAbilityTestSteps::DefaultTimeoutSeconds, [](FString& OutError)
		{
			const FString OnClient = AbilityTestChecks::FindLeftoverTags(Caster().GetClientAbilitySystem());
			const FString OnServer = AbilityTestChecks::FindLeftoverTags(Caster().GetServerAbilitySystem());
			if (OnClient.IsEmpty() && OnServer.IsEmpty())
			{
				return EResult::Next;
			}

			OutError = FString::Printf(TEXT("owning client [%s], server [%s]"), *OnClient, *OnServer);
			return EResult::Fail;
		});
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
