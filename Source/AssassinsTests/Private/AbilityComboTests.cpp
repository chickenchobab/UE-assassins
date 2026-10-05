// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityScenario.h"
#include "AssassinsTestsLog.h"

#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Assassins.Abilities.Combo.*
 *
 * The combos players use, as the guides give them, fit to the abilities of this game(Migration/PLAN_Ability_Tests.md
 * 5-2). Each step casts the way a player does: it presses again while the ability before keeps the next from starting,
 * and a recast is judged by what it does. At the end every ability has to be over with no transient tag left, and each
 * ability that has to hit has dealt damage. The damage by ability is reported; totals depend on how many attacks land
 * meanwhile, so they are not held to a number.
 *
 * AA in a name is a basic attack. It goes on as it does in a game, until the next ability stops it.
 *
 * Each combo runs again with lag and with a listen server: Assassins.Abilities.Lag100.Combo.*, .Lag200.Combo.*,
 * .Listen.Combo.*(FScenarioRegistration).
 */
namespace AbilityCombos
{
	using namespace AbilityScenarios;

	FGameplayTag Q() { return Input(TEXT("InputTag.Ability1")); }
	FGameplayTag W() { return Input(TEXT("InputTag.Ability2")); }
	FGameplayTag E() { return Input(TEXT("InputTag.Ability3")); }
	FGameplayTag R() { return Input(TEXT("InputTag.Ability4")); }

	FGameplayTag PassiveTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Status.Combo.Passive"));
	}

	void WaitUntilNearEnemy(FAbilityTestSteps& Steps, const FString& Description, double Tolerance)
	{
		Steps.WaitUntil(Description, 3.0, [Tolerance] { return IsCasterNearEnemy(Tolerance); });
	}

	// Zed's W to the spot, and the shadow there on the server.
	void ThrowShadow(FAbilityTestSteps& Steps, const FVector& Spot)
	{
		CastAbility(Steps, TEXT("W"), W(), AimAtSpot(Spot));
		Steps.WaitUntil(TEXT("The shadow stands on the server"), 3.0, [Spot]
		{
			for (TActorIterator<AAssassinsZedShadow> It(FAbilityTestSession::Get().GetServerWorld()); It; ++It)
			{
				if (FVector::Dist2D(It->GetActorLocation(), Spot) <= 100.0)
				{
					return true;
				}
			}
			return false;
		});
	}

	// Zed's W again: he swaps with the shadow that stands at the spot.
	void SwapWithShadow(FAbilityTestSteps& Steps, const FVector& Spot)
	{
		TapUntil(Steps, TEXT("W again: Zed swaps with the shadow"), W(), AimAtSpot(Spot), 5.0, [Spot] { return IsCasterNear(Spot, 150.0); });
	}

	// Zed's R at the enemy, until he stands behind it.
	void DeathMark(FAbilityTestSteps& Steps)
	{
		CastAbility(Steps, TEXT("R"), R(), AimAtEnemy());
		WaitUntilBehindEnemy(Steps, MakeShared<double>(0.0));
	}

	// Akali's E again, once the shuriken marked the enemy: she dashes to it, and the server deals the damage of the dash.
	// The damage of E as a whole does not tell: the shuriken deals some too.
	void DashToMark(FAbilityTestSteps& Steps, TSharedRef<FState> State)
	{
		TapUntil(Steps, TEXT("E again: Akali dashes to the marked enemy"), E(), AimAtEnemy(), 5.0, [] { return IsCasterNearEnemy(250.0); });
		WaitForEffectFrom(Steps, State, TEXT("E again deals the damage of the dash"), E(), TEXT("Damage_Dash"));
	}

	void WaitForFlipEnd(FAbilityTestSteps& Steps)
	{
		WaitForTag(Steps, TEXT("The flip is over"), 3.0, [] { return Caster().GetClientAbilitySystem(); },
			FGameplayTag::RequestGameplayTag(TEXT("Status.Combo.Ability3.1")), /*bPresent*/ false);
	}

	// Akali's E at the enemy, then E again to dash to it once the flip is over, as a player does. Pressed during the flip,
	// the dash cuts it short and Akali may not leave the ring of her passive.
	void ShurikenFlipAndDash(FAbilityTestSteps& Steps, TSharedRef<FState> State)
	{
		CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
		WaitForFlipEnd(Steps);
		DashToMark(Steps, State);
	}

	// Akali's passive is on once she left the ring, the attack after it is empowered and spends it.
	void EmpoweredAttack(FAbilityTestSteps& Steps, TSharedRef<FState> State)
	{
		WaitForTag(Steps, TEXT("Akali's passive is on"), 3.0, [] { return Caster().GetServerAbilitySystem(); }, PassiveTag());
		AttackOnce(Steps, State);
		WaitForTag(Steps, TEXT("The attack spends the passive"), 3.0, [] { return Caster().GetServerAbilitySystem(); }, PassiveTag(), /*bPresent*/ false);
	}

	// Walks away from the enemy, as holding the move click does, until Akali leaves the ring of her passive(400 around a
	// point by the enemy) and the passive comes on.
	void WalkOutOfRing(FAbilityTestSteps& Steps)
	{
		Steps.Step(TEXT("Akali walks out of the ring"), 5.0, [](FString& OutWaitReason)
		{
			if (HasTag(Caster().GetServerAbilitySystem(), PassiveTag()))
			{
				return EResult::Next;
			}

			const AActor* Champion = Caster().ClientChampion.Get();
			const AActor* Target = Caster().FindClientCopyOf(Enemy());
			if (Champion && Target)
			{
				FAbilityTestDriver::AddMoveInput(Caster(), Champion->GetActorLocation() - Target->GetActorLocation());
			}

			OutWaitReason = TEXT("the passive is not on yet");
			return EResult::Wait;
		});
	}

	// Akali's R again, while the recast is allowed: the recast is an ability of its own.
	void PerfectExecutionRecast(FAbilityTestSteps& Steps)
	{
		TSharedRef<TUniquePtr<FAbilityTestRecorder>> Recast = MakeShared<TUniquePtr<FAbilityTestRecorder>>();
		Steps.Do(TEXT("Watch the recast of R"), [Recast]
		{
			UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
			*Recast = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, TEXT("Akali_Ability4_Recast")));
		});
		TapAtEnemyUntil(Steps, TEXT("R again: the recast activates on the server"), R(), 10.0, [Recast] { return (*Recast)->GetNumActivations() > 0; });
	}

	// When Akali's E comes after her R. E can't start before R hits the target(Status.Combo.Ability4.1 keeps it off, so R
	// always deals its damage); after that it cuts the dash short, and Akali flips back from the enemy she faces.
	enum class EFlipTiming
	{
		// The moment R hits, the earliest the game takes E: Akali is not past the enemy yet, and the flip takes her back
		// toward where she started. With lag too: the owning client sees the hit itself, without waiting a round trip for
		// the server(UGA_Akali_Ability4::PredictTargetHit).
		RightAfterHit,

		// A moment later, once she is through the enemy: the flip takes her on, further away.
		OnceThrough,
	};

	// How far past the enemy Akali has to be for EFlipTiming::OnceThrough. The dash ends 400 past it.
	constexpr double ThroughDistance = 100.0;

	// Closer to the enemy than this along the way, Akali may face it either way and flip any way.
	constexpr double AtEnemyDistance = 50.0;

	// How far the flip has to take Akali, either way. It goes 400.
	constexpr double LeastFlipDistance = 200.0;

	double GetCasterPastEnemyOnClient()
	{
		return GetDistancePast(Caster().ClientChampion.Get(), Caster().FindClientCopyOf(Enemy()));
	}

	// Akali's R at the enemy, then her E at it while the dash goes on, and which way the flip takes her. For RightAfterHit
	// E is pressed again and again right after R, as by a player who pressed R and E at once: R turns the presses down
	// until it hits, while it waits for the server before its dash too.
	void PerfectExecutionThenFlip(FAbilityTestSteps& Steps, TSharedRef<FState> State, EFlipTiming Timing)
	{
		struct FFlip
		{
			TUniquePtr<FAbilityTestRecorder> OnClient;
			TUniquePtr<FAbilityTestRecorder> OnServer;
			bool bPressed = false;

			// How far past the enemy Akali stood on her client as E started, and once the flip was over.
			double PastAtStart = 0.0;
			double PastAtEnd = 0.0;
		};
		TSharedRef<FFlip> Flip = MakeShared<FFlip>();
		const bool bRightAfterHit = (Timing == EFlipTiming::RightAfterHit);

		CastAbility(Steps, TEXT("R"), R(), AimAtEnemy());
		Steps.Do(TEXT("E: watch for it"), [Flip]
		{
			UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
			UAbilitySystemComponent* ServerAbilitySystem = Caster().GetServerAbilitySystem();
			Flip->OnClient = MakeUnique<FAbilityTestRecorder>(ClientAbilitySystem, FindAbilityClass(ClientAbilitySystem, E()));
			Flip->OnServer = MakeUnique<FAbilityTestRecorder>(ServerAbilitySystem, FindAbilityClass(ServerAbilitySystem, E()));
		});

		const TCHAR* PressDescription = bRightAfterHit ? TEXT("E: pressed over and over, starts the moment R hits") : TEXT("E: pressed once Akali is through the enemy");
		Steps.Step(PressDescription, 3.0, [Flip, bRightAfterHit](FString& OutWaitReason)
		{
			if (Flip->bPressed)
			{
				FAbilityTestDriver::ReleaseInput(Caster(), E());
				Flip->bPressed = false;
			}

			const double Past = GetCasterPastEnemyOnClient();
			if (Flip->OnClient->GetNumActivations() > 0)
			{
				Flip->PastAtStart = Past;
				return EResult::Next;
			}

			if (!bRightAfterHit && (Past < ThroughDistance))
			{
				OutWaitReason = FString::Printf(TEXT("%.0f past the enemy"), Past);
				return EResult::Wait;
			}

			FAbilityTestDriver::SetAim(Caster(), AimAtEnemy()());
			FAbilityTestDriver::PressInput(Caster(), E());
			Flip->bPressed = true;
			OutWaitReason = Flip->OnClient->GetFailures().IsEmpty() ? FString(TEXT("pressed, not started yet")) : Flip->OnClient->GetFailures();
			return EResult::Wait;
		});

		// R keeps E off until it hits: it deals its damage first. On the server, where the damage is: with lag the owning
		// client starts E as it sees the hit, before the server hears of it.
		Steps.Step(TEXT("E starts on the server only once R hit the enemy"), 3.0, [State, Flip](FString& OutWaitReason)
		{
			if (Flip->OnServer->GetNumActivations() == 0)
			{
				OutWaitReason = FString::Printf(TEXT("E has not started on the server [%s]"), *Flip->OnServer->GetFailures());
				return EResult::Wait;
			}

			const FString Ultimate = GetNameSafe(FindAbilityClass(Caster().GetServerAbilitySystem(), R()));
			const FAbilityTestDamageLog::FEntry* Hit = State->DamageLog ? State->DamageLog->GetEntries().FindByPredicate([&Ultimate](const FAbilityTestDamageLog::FEntry& Entry)
			{
				return Entry.Ability == Ultimate;
			}) : nullptr;

			if (Hit == nullptr)
			{
				OutWaitReason = TEXT("E started, and R has not hit the enemy");
				return EResult::Fail;
			}

			const double Gap = Flip->OnServer->GetFirstActivationTime() - Hit->Time;
			OutWaitReason = FString::Printf(TEXT("E started %.3fs before R hit the enemy"), -Gap);
			return (Gap >= 0.0) ? EResult::Next : EResult::Fail;
		});

		WaitForFlipEnd(Steps);

		// Where E started: right after the hit, before the enemy, with lag too(see RightAfterHit).
		FAutomationTestBase* Test = &Steps.GetTest();
		const TCHAR* StartDescription = bRightAfterHit ? TEXT("E starts before Akali is at the enemy") : TEXT("E starts once Akali is through the enemy");
		Steps.Step(StartDescription, FAbilityTestSteps::DefaultTimeoutSeconds, [Test, Flip, bRightAfterHit](FString& OutError)
		{
			Flip->PastAtEnd = GetCasterPastEnemyOnClient();
			const FString Where = FString::Printf(TEXT("E started %.0f past the enemy, the flip ended %.0f past it"), Flip->PastAtStart, Flip->PastAtEnd);
			Test->AddInfo(Where);
			UE_LOG(LogAssassinsTests, Display, TEXT("Akali E after R | %s | %s"), *Test->GetTestFullName(), *Where);
			OutError = Where;

			if (!bRightAfterHit)
			{
				return (Flip->PastAtStart >= ThroughDistance) ? EResult::Next : EResult::Fail;
			}
			return (Flip->PastAtStart <= -AtEnemyDistance) ? EResult::Next : EResult::Fail;
		});

		// Away from the enemy she faces: back toward her start from before it, on from past it.
		Steps.Step(TEXT("The flip takes Akali away from the enemy"), FAbilityTestSteps::DefaultTimeoutSeconds, [Flip](FString& OutError)
		{
			const double Flipped = Flip->PastAtEnd - Flip->PastAtStart;
			OutError = FString::Printf(TEXT("it went %.0f, from %.0f past the enemy"), Flipped, Flip->PastAtStart);
			if (FMath::Abs(Flip->PastAtStart) < AtEnemyDistance)
			{
				return EResult::Next;
			}
			return ((Flip->PastAtStart < 0.0) ? (Flipped <= -LeastFlipDistance) : (Flipped >= LeastFlipDistance)) ? EResult::Next : EResult::Fail;
		});

		// The server flips her the same way.
		WaitUntilNear(Steps, TEXT("The server has Akali where her client does"), 1.0, [] { return Caster().ServerChampion.Get(); }, []
		{
			const AActor* ClientChampion = Caster().ClientChampion.Get();
			return ClientChampion ? ClientChampion->GetActorLocation() : FVector::ZeroVector;
		}, 50.0);
	}

// Zed, closing in: W toward an enemy out of reach, W again to swap in, Q, E, AA.
static FScenarioRegistration ZedWWQEAACombo(TEXT("Combo"), TEXT("Zed.WWQEAA"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector ShadowSpot = SpotTowardEnemy(450.0);
	MovePlayer(Steps, 1, SpotTowardEnemy(650.0));
	ThrowShadow(Steps, ShadowSpot);
	SwapWithShadow(Steps, ShadowSpot);
	ThrowShurikens(Steps, State, 2);
	CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
	AttackOnce(Steps, State);
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Input(TEXT("InputTag.Attack")));
});

// Zed, the triple Q: W beside the enemy, R(a second shadow at the start), E, Q from Zed and both shadows, AA, R again
// back to the start. The mark bursts meanwhile.
static FScenarioRegistration ZedTripleQCombo(TEXT("Combo"), TEXT("Zed.TripleQ"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	ThrowShadow(Steps, SpotBesideEnemy(200.0));
	DeathMark(Steps);
	CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
	ThrowShurikens(Steps, State, 3);
	AttackOnce(Steps, State);
	TapUntil(Steps, TEXT("R again: Zed goes back to the shadow at the start"), R(), AimAtEnemy(), 5.0, []
	{
		return IsCasterNear(FAbilityTestSession::GetStartLocation(0), 150.0);
	});
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, Input(TEXT("InputTag.Attack")));
	CheckDamageFrom(Steps, State, R());
});

// Zed, as given: R, W beside the enemy, E, Q(three throw), W again to swap out, AA.
static FScenarioRegistration ZedRWEQWAACombo(TEXT("Combo"), TEXT("Zed.RWEQWAA"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector ShadowSpot = SpotBesideEnemy(200.0);
	DeathMark(Steps);
	ThrowShadow(Steps, ShadowSpot);
	CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
	ThrowShurikens(Steps, State, 3);
	SwapWithShadow(Steps, ShadowSpot);
	AttackOnce(Steps, State);
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, Input(TEXT("InputTag.Attack")));
	CheckDamageFrom(Steps, State, R());
});

// Zed, as given(W placed, then R W E Q): W beside the enemy, R, W again to swap to the shadow, E, Q(three throw).
static FScenarioRegistration ZedWRWEQCombo(TEXT("Combo"), TEXT("Zed.WRWEQ"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector ShadowSpot = SpotBesideEnemy(200.0);
	ThrowShadow(Steps, ShadowSpot);
	DeathMark(Steps);
	SwapWithShadow(Steps, ShadowSpot);
	CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
	ThrowShurikens(Steps, State, 3);
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, R());
});

// Leblanc, the single target burst: Q, R(the mimic of Q pops the mark), W onto the enemy, E, which roots.
static FScenarioRegistration LeblancQRWECombo(TEXT("Combo"), TEXT("Leblanc.QRWE"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAbility(Steps, TEXT("Q"), Q(), AimAtEnemy());
	CastAbility(Steps, TEXT("R"), R(), AimAtEnemy(), TEXT("Leblanc_Ability1_Mimic"));
	CastAbility(Steps, TEXT("W"), W(), AimAtEnemy());
	WaitUntilNearEnemy(Steps, TEXT("Leblanc lands by the enemy"), 200.0);
	CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
	Steps.WaitUntil(TEXT("The enemy gets rooted"), 5.0, [State] { return State->bEnemyRootedSeen; });
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, TEXT("Leblanc_Ability1_Mimic"));
	CheckDamageFrom(Steps, State, W());
	CheckDamageFrom(Steps, State, E());
});

// Leblanc, from afar: W toward an enemy out of reach, R(the mimic of W lands on it), Q, E, which roots.
static FScenarioRegistration LeblancWRQECombo(TEXT("Combo"), TEXT("Leblanc.WRQE"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	MovePlayer(Steps, 1, SpotTowardEnemy(700.0));
	CastAbility(Steps, TEXT("W"), W(), AimAtSpot(SpotTowardEnemy(450.0)));
	Steps.WaitUntil(TEXT("Leblanc lands on the way"), 3.0, [] { return IsCasterNear(SpotTowardEnemy(450.0), 100.0); });
	CastAbility(Steps, TEXT("R"), R(), AimAtEnemy(), TEXT("Leblanc_Ability2_Mimic"));
	WaitUntilNearEnemy(Steps, TEXT("The mimic of W lands by the enemy"), 200.0);
	CastAbility(Steps, TEXT("Q"), Q(), AimAtEnemy());
	CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
	Steps.WaitUntil(TEXT("The enemy gets rooted"), 5.0, [State] { return State->bEnemyRootedSeen; });
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, TEXT("Leblanc_Ability2_Mimic"));
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, E());
});

// Leblanc, chain first(as given, and in the guides): E, Q, R(the mimic of Q), W onto the enemy. The chain roots.
static FScenarioRegistration LeblancEQRWCombo(TEXT("Combo"), TEXT("Leblanc.EQRW"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAbility(Steps, TEXT("E"), E(), AimAtEnemy());
	WaitForTag(Steps, TEXT("The chain tethers the enemy"), 5.0, [] { return Enemy().GetServerAbilitySystem(); },
		FGameplayTag::RequestGameplayTag(TEXT("Status.Target.Leblanc.Tether.Ability3")));
	CastAbility(Steps, TEXT("Q"), Q(), AimAtEnemy());
	CastAbility(Steps, TEXT("R"), R(), AimAtEnemy(), TEXT("Leblanc_Ability1_Mimic"));
	CastAbility(Steps, TEXT("W"), W(), AimAtEnemy());
	WaitUntilNearEnemy(Steps, TEXT("Leblanc lands by the enemy"), 200.0);
	Steps.WaitUntil(TEXT("The enemy gets rooted"), 5.0, [State] { return State->bEnemyRootedSeen; });
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, TEXT("Leblanc_Ability1_Mimic"));
	CheckDamageFrom(Steps, State, W());
});

// Leblanc, the poke: Q, W onto the enemy, AA, W again back to the start.
static FScenarioRegistration LeblancQWAAWCombo(TEXT("Combo"), TEXT("Leblanc.QWAAW"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAbility(Steps, TEXT("Q"), Q(), AimAtEnemy());
	CastAbility(Steps, TEXT("W"), W(), AimAtEnemy());
	WaitUntilNearEnemy(Steps, TEXT("Leblanc lands by the enemy"), 200.0);
	AttackOnce(Steps, State);
	TapUntil(Steps, TEXT("W again: Leblanc returns to her start"), W(), AimAtEnemy(), 5.0, []
	{
		return IsCasterNear(FAbilityTestSession::GetStartLocation(0), 100.0);
	});
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, W());
	CheckDamageFrom(Steps, State, Input(TEXT("InputTag.Attack")));
});

// Akali, the trade: Q(the ring comes), E(the flip takes her out: the passive is on), E again onto the enemy, the empowered
// AA, Q once more.
static FScenarioRegistration AkaliQEEAAQCombo(TEXT("Combo"), TEXT("Akali.QEEAAQ"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAbility(Steps, TEXT("Q"), Q(), AimAtEnemy());
	ShurikenFlipAndDash(Steps, State);
	EmpoweredAttack(Steps, State);
	CastAbility(Steps, TEXT("Q again, once off cooldown"), Q(), AimAtEnemy(), nullptr, 10.0);
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Input(TEXT("InputTag.Attack")));
});

// Akali, the dive: R at the enemy and E the moment it hits(without lag the flip takes her back toward her start), E again
// onto the enemy, the empowered AA, Q, R again. The flip may leave her just inside the ring of her passive: she walks out.
static FScenarioRegistration AkaliREEAAQRCombo(TEXT("Combo"), TEXT("Akali.REEAAQR"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	PerfectExecutionThenFlip(Steps, State, EFlipTiming::RightAfterHit);
	WalkOutOfRing(Steps);
	DashToMark(Steps, State);
	EmpoweredAttack(Steps, State);
	CastAbility(Steps, TEXT("Q"), Q(), AimAtEnemy());
	PerfectExecutionRecast(Steps);
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, R());
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Input(TEXT("InputTag.Attack")));
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, TEXT("Akali_Ability4_Recast"));
});

// Akali, the long all-in as given: R, E a moment later once she is through the enemy(the flip takes her on, further
// away), E again, the empowered AA, Q, W, Q once more, out of the ring and the empowered AA again, R again while it is
// still allowed.
static FScenarioRegistration AkaliREEAAQWQAARCombo(TEXT("Combo"), TEXT("Akali.REEAAQWQAAR"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	PerfectExecutionThenFlip(Steps, State, EFlipTiming::OnceThrough);
	WalkOutOfRing(Steps);
	DashToMark(Steps, State);
	EmpoweredAttack(Steps, State);
	CastAbility(Steps, TEXT("Q"), Q(), AimAtEnemy());
	CastAbility(Steps, TEXT("W"), W(), AimAtEnemy());
	CastAbility(Steps, TEXT("Q again, once off cooldown"), Q(), AimAtEnemy(), nullptr, 10.0);
	WalkOutOfRing(Steps);
	EmpoweredAttack(Steps, State);
	PerfectExecutionRecast(Steps);
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, R());
	CheckDamageFrom(Steps, State, E());
	CheckDamageFrom(Steps, State, Q());
	CheckDamageFrom(Steps, State, Input(TEXT("InputTag.Attack")));
	CheckDamageFrom(Steps, State, TEXT("Akali_Ability4_Recast"));
});

// The combos above, and again with lag on the clients and with the caster hosting a listen server. The variants are named
// apart from Combo, so that running Combo doesn't run them too.
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FComboTests, "Assassins.Abilities.Combo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FComboLag100Tests, "Assassins.Abilities.Lag100.Combo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FComboLag200Tests, "Assassins.Abilities.Lag200.Combo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FComboListenTests, "Assassins.Abilities.Listen.Combo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FComboTests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Combo"), OutBeautifiedNames, OutTestCommands);
}

bool FComboTests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Combo"), Parameters, /*bListenServer*/ false, /*LagMs*/ 0);
}

void FComboLag100Tests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Combo"), OutBeautifiedNames, OutTestCommands, /*bVariant*/ true);
}

bool FComboLag100Tests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Combo"), Parameters, /*bListenServer*/ false, /*LagMs*/ 100);
}

void FComboLag200Tests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Combo"), OutBeautifiedNames, OutTestCommands, /*bVariant*/ true);
}

bool FComboLag200Tests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Combo"), Parameters, /*bListenServer*/ false, /*LagMs*/ 200);
}

void FComboListenTests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Combo"), OutBeautifiedNames, OutTestCommands, /*bVariant*/ true);
}

bool FComboListenTests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Combo"), Parameters, /*bListenServer*/ true, /*LagMs*/ 0);
}

} // namespace AbilityCombos

#endif // WITH_DEV_AUTOMATION_TESTS
