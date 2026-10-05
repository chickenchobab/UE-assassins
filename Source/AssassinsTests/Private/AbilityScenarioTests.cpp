// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityScenario.h"
#include "AssassinsTestsLog.h"

#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "AbilitySystemGlobals.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"
#include "Character/Champions/Akali/AssassinsShroud.h"
#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "Player/AssassinsPlayerController.h"
#include "Player/AssassinsPlayerState.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Assassins.Abilities.Scenario.*
 *
 * What the smoke tests can't see: recasts, combinations, abilities that need the enemy close, crowd control, Leblanc's
 * R as a new session has it, the basic attack going on and changing targets, an ability or a recast pressed while
 * another is being cast(...DuringQCast) or while a unit targeted one waits for the server(Akali.QDuringRWait). Damage
 * is checked against what was measured(±1%) where a measurement exists; elsewhere it only has to be dealt.
 *
 * Each scenario runs again with lag and with a listen server: Assassins.Abilities.Lag100.Scenario.*, .Lag200.Scenario.*,
 * .Listen.Scenario.*(FScenarioRegistration). Not the ones that only look at the server(the aggro).
 *
 * Assassins.Abilities.SelfCheck.* make a check fail on purpose and expect its error: they pass only if the steps do
 * report failures.
 */
namespace AbilityScenarioTests
{
	using namespace AbilityScenarios;

	// In the session of four(Attack.Retarget), the other player of the enemy team.
	constexpr int32 OtherEnemy = 3;

	bool IsShadowOnServer()
	{
		return CountActors<AAssassinsZedShadow>(FAbilityTestSession::Get().GetServerWorld()) > 0;
	}

	const UAbilitySystemComponent* CasterOnServer()
	{
		return Caster().GetServerAbilitySystem();
	}

	const UAbilitySystemComponent* EnemyOnServer()
	{
		return Enemy().GetServerAbilitySystem();
	}

	// A box of world static in this world only: it does not replicate.
	AActor* SpawnWall(UWorld* World, const FVector& Center, const FVector& Scale)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Wall = World ? World->SpawnActor<AStaticMeshActor>(Center, FRotator::ZeroRotator, Params) : nullptr;
		if (Wall == nullptr)
		{
			return nullptr;
		}

		Wall->SetReplicates(false);
		UStaticMeshComponent* Mesh = Wall->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Wall->SetActorScale3D(Scale);
		return Wall;
	}

	// Whether the champion's capsule is inside world static, short of touching it.
	bool IsInsideWorldStatic(const AAssassinsChampion* Champion)
	{
		const UCapsuleComponent* Capsule = Champion ? Champion->GetCapsuleComponent() : nullptr;
		if (Capsule == nullptr)
		{
			return false;
		}

		const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AbilityTestInsideWorldStatic), /*bTraceComplex*/ false, Champion);
		FCollisionResponseParams ResponseParams;
		ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
		return Champion->GetWorld()->OverlapBlockingTestByChannel(Capsule->GetComponentLocation(), Capsule->GetComponentQuat(), ECC_Pawn,
			Capsule->GetCollisionShape(/*Inflation*/ -1.0f), QueryParams, ResponseParams);
	}

	bool IsAvoiding(const AAssassinsChampion* Champion)
	{
		return Champion && Champion->GetCharacterMovement()->bUseRVOAvoidance;
	}

	// Takes all of the player's health on the server with an effect, as a blow does: the champion dies as in a game. It
	// does not come back, and the next test brings up a new play session(FAbilityTestSession::IsReadyFor).
	void KillOnServer(const FAbilityTestPlayer& Player)
	{
		UAbilitySystemComponent* AbilitySystem = Player.GetServerAbilitySystem();
		if (AbilitySystem == nullptr)
		{
			return;
		}

		UGameplayEffect* Kill = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
		Kill->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Kill->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UAssassinsHealthSet::GetHealthAttribute();
		Modifier.ModifierOp = EGameplayModOp::Override;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.f));
		AbilitySystem->ApplyGameplayEffectToSelf(Kill, /*Level*/ 1.f, AbilitySystem->MakeEffectContext());
	}

	// Changes the target's health on the server by Amount with an effect: a heal above zero(Healing), a blow below
	// (TrueDamage). From the source player's champion, as its abilities do(UAssassinsGameplayAbility::MakeEffectContext),
	// or from no champion when there is no source.
	void ChangeHealthOnServer(const FAbilityTestPlayer* Source, UAbilitySystemComponent* TargetAbilitySystem, float Amount)
	{
		UAbilitySystemComponent* SourceAbilitySystem = Source ? Source->GetServerAbilitySystem() : TargetAbilitySystem;
		if ((SourceAbilitySystem == nullptr) || (TargetAbilitySystem == nullptr))
		{
			return;
		}

		UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = (Amount > 0.f) ? UAssassinsHealthSet::GetHealingAttribute() : UAssassinsHealthSet::GetTrueDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(FMath::Abs(Amount)));

		// The ability system's own context has its player state for instigator, which is no champion.
		FGameplayEffectContextHandle Context = SourceAbilitySystem->MakeEffectContext();
		if (Source)
		{
			Context.AddInstigator(Source->ServerChampion.Get(), Source->ServerChampion.Get());
		}
		SourceAbilitySystem->ApplyGameplayEffectToTarget(Effect, TargetAbilitySystem, /*Level*/ 1.f, Context);
	}

	void ChangeHealthOnServer(const FAbilityTestPlayer* Source, const FAbilityTestPlayer& Target, float Amount)
	{
		ChangeHealthOnServer(Source, Target.GetServerAbilitySystem(), Amount);
	}

	// A minion in the server world, with no controller: it stands where it is put, and only takes what it is given. The
	// test map has no minions of its own.
	AActor* SpawnMinionOnServer(const FVector& FloorLocation)
	{
		UWorld* World = FAbilityTestSession::Get().GetServerWorld();
		UClass* MinionClass = LoadClass<APawn>(nullptr, TEXT("/Game/Minions/Blueprints/Melee/Blue/B_MeleeMinion_Blue.B_MeleeMinion_Blue_C"));
		if ((World == nullptr) || (MinionClass == nullptr))
		{
			return nullptr;
		}

		const FTransform SpawnTransform(FRotator::ZeroRotator, FloorLocation + FVector(0.0, 0.0, 100.0));
		APawn* Minion = World->SpawnActorDeferred<APawn>(MinionClass, SpawnTransform, /*Owner*/ nullptr, /*Instigator*/ nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Minion == nullptr)
		{
			return nullptr;
		}

		Minion->AutoPossessAI = EAutoPossessAI::Disabled;
		Minion->FinishSpawning(SpawnTransform);
		return Minion;
	}

	// The enemy dies on the server(KillOnServer). Its death event carries the effect, which the server can't name to the
	// clients: the effect is the test's own, made on the spot.
	void KillEnemy(FAbilityTestSteps& Steps)
	{
		Steps.GetTest().AddExpectedMessagePlain(TEXT("FNetGUIDCache::SupportsObject: GameplayEffect /Engine/Transient."),
			ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences, any or none*/ -1);
		Steps.Do(TEXT("The enemy dies"), [] { KillOnServer(Enemy()); });
	}

	// The death of the enemy plays out to its end on the server, and its champion goes away(UGA_Death).
	void WaitForEnemyToGo(FAbilityTestSteps& Steps)
	{
		Steps.WaitUntil(TEXT("The enemy's death plays out and its champion goes away on the server"), 10.0, []
		{
			return !Enemy().ServerChampion.IsValid();
		});
	}

	// The shadows a skill state holds that are still there. One destroyed without taking itself out(as a reset does) keeps
	// its place until the next shadow comes, which is allowed(UAssassinsChampionSkillState_Zed::AddShadow).
	int32 CountLiveShadowsInSkillState(AAssassinsChampion* Champion)
	{
		const UAssassinsChampionSkillState_Zed* SkillState = Champion ? Champion->GetSkillState<UAssassinsChampionSkillState_Zed>() : nullptr;
		if (SkillState == nullptr)
		{
			return INDEX_NONE;
		}

		int32 Count = 0;
		for (const TObjectPtr<AAssassinsZedShadow>& Shadow : SkillState->GetShadows())
		{
			Count += IsValid(Shadow) ? 1 : 0;
		}
		return Count;
	}

	// Zed's mark bursts about 3 s after the strike with the damage he dealt meanwhile, after the target's resistances(it
	// is less than what he dealt). He attacks after the strike, so there is some.
	void WaitForDeathMarkBurst(FAbilityTestSteps& Steps, TSharedRef<FState> State, TSharedRef<double> StrikeTime)
	{
		Steps.Step(TEXT("The mark bursts about 3 s after the strike"), 8.0, [State, StrikeTime](FString& OutError)
		{
			const FString Ultimate = GetNameSafe(FindAbilityClass(Caster().GetServerAbilitySystem(), Input(TEXT("InputTag.Ability4"))));
			const TArray<FAbilityTestDamageLog::FEntry>& Entries = State->DamageLog->GetEntries();

			// The burst is the ultimate's effect well after the strike.
			const FAbilityTestDamageLog::FEntry* Burst = Entries.FindByPredicate([&Ultimate, StrikeTime](const FAbilityTestDamageLog::FEntry& Entry)
			{
				return (Entry.Ability == Ultimate) && (Entry.Time >= *StrikeTime + 1.0);
			});
			if (Burst == nullptr)
			{
				OutError = TEXT("no burst yet");
				return EResult::Wait;
			}

			float DealtMeanwhile = 0.f;
			for (const FAbilityTestDamageLog::FEntry& Entry : Entries)
			{
				if ((&Entry != Burst) && (Entry.Time >= *StrikeTime - 0.5) && (Entry.Time < Burst->Time))
				{
					DealtMeanwhile += Entry.Damage;
				}
			}

			const double Delay = Burst->Time - *StrikeTime;
			UE_LOG(LogAssassinsTests, Display, TEXT("Zed R burst: %.1f after %.2fs, dealt meanwhile %.1f"), Burst->Damage, Delay, DealtMeanwhile);
			if ((Delay < 2.5) || (Delay > 4.5))
			{
				OutError = FString::Printf(TEXT("burst %.2fs after the strike"), Delay);
				return EResult::Fail;
			}

			if ((DealtMeanwhile > 0.f) && (Burst->Damage <= 0.f))
			{
				OutError = FString::Printf(TEXT("the burst dealt nothing, Zed dealt %.1f meanwhile"), DealtMeanwhile);
				return EResult::Fail;
			}
			return EResult::Next;
		});
	}

	// The caster's ability the input starts, on its owning client.
	const FGameplayAbilitySpec* FindClientSpec(const FGameplayTag& InputTag)
	{
		const UAbilitySystemComponent* AbilitySystem = Caster().GetClientAbilitySystem();
		if (AbilitySystem)
		{
			for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
				{
					return &Spec;
				}
			}
		}
		return nullptr;
	}

	// A press of the input right as the next cast begins on the owning client: Status.Channeling comes on. A press that
	// looked for the cast every frame could miss a short one: at a few frames a second, a cast that waited for the server
	// began and ended between two looks(Migration/ISSUES.md 93). The beginning is caught as it happens, and the press
	// follows on the next step of the test: pressed right then, inside the input handling that started the cast, the press
	// would be cleared with that handling's inputs. The controller takes it on the next frame, as it takes a player's.
	struct FPressAtCastStart
	{
		FGameplayTag InputTag;
		TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
		FDelegateHandle Handle;
		bool bCastBegun = false;
		bool bPressed = false;

		// Whether the tags kept the input's ability off as it was pressed.
		bool bBlockedByTags = false;

		void Disarm()
		{
			if (UAbilitySystemComponent* ArmedAbilitySystem = AbilitySystem.Get())
			{
				ArmedAbilitySystem->RegisterGameplayTagEvent(AssassinsGameplayTags::Status_Channeling, EGameplayTagEventType::NewOrRemoved).Remove(Handle);
			}
			Handle.Reset();
		}
	};

	// A press of an ability as the dash going on ends on the owning client(Status.Dashing goes), and what it started.
	struct FPressAtLanding
	{
		TUniquePtr<FAbilityTestRecorder> OnClient;
		TUniquePtr<FAbilityTestRecorder> OnServer;
		TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
		FDelegateHandle Handle;
		bool bPressed = false;

		void Disarm()
		{
			if (UAbilitySystemComponent* ArmedAbilitySystem = AbilitySystem.Get())
			{
				ArmedAbilitySystem->RegisterGameplayTagEvent(AssassinsGameplayTags::Status_Dashing, EGameplayTagEventType::NewOrRemoved).Remove(Handle);
			}
			Handle.Reset();
		}
	};

	// Arms the press, then casts the ability of CastInputTag at the enemy. The press comes as that cast begins.
	TSharedRef<FPressAtCastStart> CastWithPressAtItsStart(FAbilityTestSteps& Steps, const FString& CastName, const FGameplayTag& CastInputTag, const FString& Name, const FGameplayTag& InputTag)
	{
		TSharedRef<FPressAtCastStart> Press = MakeShared<FPressAtCastStart>();
		Press->InputTag = InputTag;

		Steps.Do(FString::Printf(TEXT("%s: press it as the cast of %s begins"), *Name, *CastName), [Press]
		{
			UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
			if (ClientAbilitySystem == nullptr)
			{
				return;
			}

			Press->AbilitySystem = ClientAbilitySystem;
			TWeakPtr<FPressAtCastStart> WeakPress = Press;
			Press->Handle = ClientAbilitySystem->RegisterGameplayTagEvent(AssassinsGameplayTags::Status_Channeling, EGameplayTagEventType::NewOrRemoved).AddLambda([WeakPress](const FGameplayTag Tag, int32 NewCount)
			{
				const TSharedPtr<FPressAtCastStart> ArmedPress = WeakPress.Pin();
				if (ArmedPress.IsValid() && (NewCount > 0))
				{
					ArmedPress->bCastBegun = true;
				}
			});
		});
		Steps.Finally([Press] { Press->Disarm(); });

		Steps.EachFrame([Press]
		{
			if (!Press->bCastBegun || Press->bPressed)
			{
				return;
			}

			const UAbilitySystemComponent* ClientAbilitySystem = Press->AbilitySystem.Get();
			const FGameplayAbilitySpec* Spec = FindClientSpec(Press->InputTag);
			Press->bBlockedByTags = ClientAbilitySystem && Spec && !Spec->Ability->DoesAbilitySatisfyTagRequirements(*ClientAbilitySystem);

			FAbilityTestDriver::SetAim(Caster(), AimAtEnemy()());
			FAbilityTestDriver::PressInput(Caster(), Press->InputTag);
			Press->bPressed = true;
		});

		CastAbility(Steps, CastName, CastInputTag, AimAtEnemy());

		// A cast that waits for the server(a unit targeted one) begins a little after its ability starts.
		Steps.Step(FString::Printf(TEXT("%s: pressed as the cast began"), *Name), 3.0, [Press](FString& OutWaitReason)
		{
			OutWaitReason = TEXT("the cast has not begun");
			return Press->bPressed ? EResult::Next : EResult::Wait;
		});
		Steps.Do(FString::Printf(TEXT("%s: let it go"), *Name), [Press]
		{
			Press->Disarm();
			FAbilityTestDriver::ReleaseInput(Caster(), Press->InputTag);
		});
		return Press;
	}

	// Casts the ability of CastInputTag at the enemy, presses the input once as that cast begins(the cast holds
	// Status.Channeling on the owning client), then waits the cast out. The input's ability must not start, on the owning
	// client or the server: the cast's tags keep it off, and nothing else does, since it can start once the cast is over.
	// Before the cast blocked them(Migration/RULES.md 6단계, "시전 중 다른 스킬 막기"), these abilities started and cut the
	// cast short, or were spent for nothing.
	void CastAndPressDuringIt(FAbilityTestSteps& Steps, const FString& CastName, const FGameplayTag& CastInputTag, const FString& Name, const FGameplayTag& InputTag)
	{
		struct FRecorders
		{
			TUniquePtr<FAbilityTestRecorder> OnClient;
			TUniquePtr<FAbilityTestRecorder> OnServer;
		};
		TSharedRef<FRecorders> Recorders = MakeShared<FRecorders>();

		Steps.Do(FString::Printf(TEXT("%s: watch for it"), *Name), [Recorders, InputTag]
		{
			UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
			UAbilitySystemComponent* ServerAbilitySystem = Caster().GetServerAbilitySystem();
			Recorders->OnClient = MakeUnique<FAbilityTestRecorder>(ClientAbilitySystem, FindAbilityClass(ClientAbilitySystem, InputTag));
			Recorders->OnServer = MakeUnique<FAbilityTestRecorder>(ServerAbilitySystem, FindAbilityClass(ServerAbilitySystem, InputTag));
		});

		TSharedRef<FPressAtCastStart> Press = CastWithPressAtItsStart(Steps, CastName, CastInputTag, Name, InputTag);

		WaitForTag(Steps, TEXT("The cast goes on to its end"), 5.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Channeling, /*bPresent*/ false);
		Steps.WaitFor(TEXT("Let a press reach the server"), 0.3);

		Steps.Step(FString::Printf(TEXT("%s did not start during the cast"), *Name), FAbilityTestSteps::DefaultTimeoutSeconds, [Recorders, Press](FString& OutError)
		{
			const int32 OnClient = Recorders->OnClient->GetNumActivations();
			const int32 OnServer = Recorders->OnServer->GetNumActivations();
			if ((OnClient > 0) || (OnServer > 0))
			{
				OutError = FString::Printf(TEXT("it started %d time(s) on the owning client, %d on the server"), OnClient, OnServer);
				return EResult::Fail;
			}

			// The press has to have been turned down, by the tags.
			if (Recorders->OnClient->GetFailures().IsEmpty() || !Press->bBlockedByTags)
			{
				OutError = Recorders->OnClient->GetFailures().IsEmpty() ? TEXT("the press was never tried") : TEXT("the press was turned down, but not by the tags");
				return EResult::Fail;
			}
			return EResult::Next;
		});

		Steps.WaitUntil(FString::Printf(TEXT("%s can start once the cast is over"), *Name), 3.0, [InputTag]
		{
			const UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
			const FGameplayAbilitySpec* Spec = FindClientSpec(InputTag);
			return ClientAbilitySystem && Spec && Spec->Ability->CanActivateAbility(Spec->Handle, ClientAbilitySystem->AbilityActorInfo.Get());
		});
	}

	// Casts the ability of CastInputTag at the enemy and presses the input once as that cast begins, for the recast of an
	// ability that is already running, which no tag keeps off. Both sides wait for this press, so the owning client must
	// not take it, nor send it to the server(UAbilityTask_WaitRecastPress): the recast must not happen on either side
	// until the cast is over. Recasted tells whether it did, and is watched every frame from the press on, as a recast may
	// come and go between two looks.
	void CastAndPressRecastDuringIt(FAbilityTestSteps& Steps, const FString& CastName, const FGameplayTag& CastInputTag, const FString& Name, const FGameplayTag& InputTag, TFunction<bool()> Recasted)
	{
		struct FWatch
		{
			bool bWatching = true;
			bool bRecasted = false;
		};
		TSharedRef<FWatch> Watch = MakeShared<FWatch>();

		Steps.EachFrame([Watch, Recasted]
		{
			if (Watch->bWatching)
			{
				Watch->bRecasted = Watch->bRecasted || Recasted();
			}
		});

		CastWithPressAtItsStart(Steps, CastName, CastInputTag, Name, InputTag);

		WaitForTag(Steps, TEXT("The cast goes on to its end"), 5.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Channeling, /*bPresent*/ false);
		Steps.WaitFor(TEXT("Let a press reach the server"), 0.3);

		Steps.Step(FString::Printf(TEXT("%s did not recast during the cast"), *Name), FAbilityTestSteps::DefaultTimeoutSeconds, [Watch](FString& OutError)
		{
			Watch->bWatching = false;
			OutError = TEXT("it recast, on the owning client or the server");
			return Watch->bRecasted ? EResult::Fail : EResult::Next;
		});
	}

// Zed's E reaches 232 around him: the enemy stands at 150.
static FScenarioRegistration ZedEInRangeScenario(TEXT("Scenario"), TEXT("Zed.EInRange"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	MovePlayer(Steps, 1, FAbilityTestSession::GetStartLocation(0) + FVector(0.0, 150.0, 0.0));
	TapAtEnemy(Steps, Input(TEXT("InputTag.Ability3")));
	CheckDamage(Steps, State, 1.5, 220.4f);
});

// Zed and the shadow both reach the enemy with E: it is hit once.
static FScenarioRegistration ZedWThenEScenario(TEXT("Scenario"), TEXT("Zed.WThenE"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector Near = FAbilityTestSession::GetStartLocation(0) + FVector(0.0, 150.0, 0.0);
	MovePlayer(Steps, 1, Near);
	TapAtSpot(Steps, Input(TEXT("InputTag.Ability2")), Near + FVector(150.0, 0.0, 0.0));
	Steps.WaitUntil(TEXT("A shadow stands on the server"), 3.0, [] { return IsShadowOnServer(); });
	Steps.WaitFor(TEXT("Let the shadow arrive"), 1.0);
	TapAtEnemy(Steps, Input(TEXT("InputTag.Ability3")));
	CheckDamage(Steps, State, 1.5, 220.4f);
});

// Q and E pressed together with a shadow out: both end on both sides, and the shadow throws and slashes along.
static FScenarioRegistration ZedQAndEScenario(TEXT("Scenario"), TEXT("Zed.QAndE"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Q = Input(TEXT("InputTag.Ability1"));
	const FGameplayTag E = Input(TEXT("InputTag.Ability3"));

	TapAtSpot(Steps, Input(TEXT("InputTag.Ability2")), SpotBesideEnemy(200.0));
	Steps.WaitUntil(TEXT("A shadow stands on the server"), 3.0, [] { return IsShadowOnServer(); });
	Steps.WaitFor(TEXT("Let the shadow arrive"), 1.0);

	struct FRecorders
	{
		TUniquePtr<FAbilityTestRecorder> ServerQ;
		TUniquePtr<FAbilityTestRecorder> ServerE;
		TUniquePtr<FAbilityTestRecorder> ClientQ;
		TUniquePtr<FAbilityTestRecorder> ClientE;
	};
	TSharedRef<FRecorders> Recorders = MakeShared<FRecorders>();
	Steps.Do(TEXT("Watch Q and E"), [Recorders, State, Q, E]
	{
		UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
		UAbilitySystemComponent* Client = Caster().GetClientAbilitySystem();
		Recorders->ServerQ = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, Q));
		Recorders->ServerE = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, E));
		Recorders->ClientQ = MakeUnique<FAbilityTestRecorder>(Client, FindAbilityClass(Client, Q));
		Recorders->ClientE = MakeUnique<FAbilityTestRecorder>(Client, FindAbilityClass(Client, E));
		State->MostProjectiles = 0;
	});
	Steps.Do(TEXT("Press Q"), [Q]
	{
		FAbilityTestDriver::SetAim(Caster(), FAbilityTestDriver::AimAt(Caster().FindClientCopyOf(Enemy())));
		FAbilityTestDriver::PressInput(Caster(), Q);
	});
	Steps.Do(TEXT("Let Q go, press E"), [Q, E]
	{
		FAbilityTestDriver::ReleaseInput(Caster(), Q);
		FAbilityTestDriver::PressInput(Caster(), E);
	});
	Steps.Do(TEXT("Let E go"), [E]
	{
		FAbilityTestDriver::ReleaseInput(Caster(), E);
	});

	Steps.Step(TEXT("Q and E activate on the server"), 5.0, [Recorders](FString& OutWaitReason)
	{
		if ((Recorders->ServerQ->GetNumActivations() > 0) && (Recorders->ServerE->GetNumActivations() > 0))
		{
			return EResult::Next;
		}

		OutWaitReason = FString::Printf(TEXT("Q %d, E %d activations [%s] [%s]"), Recorders->ServerQ->GetNumActivations(), Recorders->ServerE->GetNumActivations(),
			*Recorders->ServerQ->GetFailures(), *Recorders->ServerE->GetFailures());
		return EResult::Wait;
	});

	Steps.WaitFor(TEXT("Let the shurikens fly"), 1.5);
	Steps.Step(TEXT("Zed and the shadow both throw"), 1.0, [State](FString& OutError)
	{
		OutError = FString::Printf(TEXT("at most %d shurikens at once on the server"), State->MostProjectiles);
		return (State->MostProjectiles >= 2) ? EResult::Next : EResult::Fail;
	});

	Steps.Step(TEXT("Q and E end on the owning client and the server"), 10.0, [Recorders](FString& OutWaitReason)
	{
		TArray<FString> Running;
		if (Recorders->ServerQ->IsActive()) { Running.Add(TEXT("Q on the server")); }
		if (Recorders->ServerE->IsActive()) { Running.Add(TEXT("E on the server")); }
		if (Recorders->ClientQ->IsActive()) { Running.Add(TEXT("Q on the owning client")); }
		if (Recorders->ClientE->IsActive()) { Running.Add(TEXT("E on the owning client")); }
		if (Running.IsEmpty())
		{
			return EResult::Next;
		}

		OutWaitReason = FString::Printf(TEXT("still running: %s"), *FString::Join(Running, TEXT(", ")));
		return EResult::Wait;
	});

	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, Q, 596.4f);
	CheckDamageFrom(Steps, State, E, 220.4f);
});

// R: Zed reappears behind the enemy; about 3 s later the mark bursts with the damage he dealt it meanwhile(his attacks
// go on after the strike); R again brings him back to the shadow he left.
static FScenarioRegistration ZedRScenario(TEXT("Scenario"), TEXT("Zed.R"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));
	TSharedRef<double> StrikeTime = MakeShared<double>(0.0);

	CastAbility(Steps, TEXT("R"), R, AimAtEnemy());
	WaitUntilBehindEnemy(Steps, StrikeTime);
	WaitForDeathMarkBurst(Steps, State, StrikeTime);
	TapUntil(Steps, TEXT("R again: Zed goes back to his shadow"), R, AimAtEnemy(), 5.0, []
	{
		return IsCasterNear(FAbilityTestSession::GetStartLocation(0), 100.0);
	});
	FinishCombo(Steps);
});

// R, an attack, then R again before the mark bursts, as players often do: the mark still bursts.
static FScenarioRegistration ZedRRecastBeforeBurstScenario(TEXT("Scenario"), TEXT("Zed.RRecastBeforeBurst"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));
	TSharedRef<double> StrikeTime = MakeShared<double>(0.0);

	CastAbility(Steps, TEXT("R"), R, AimAtEnemy());
	WaitUntilBehindEnemy(Steps, StrikeTime);
	AttackOnce(Steps, State);
	TapUntil(Steps, TEXT("R again: Zed goes back to his shadow"), R, AimAtEnemy(), 5.0, []
	{
		return IsCasterNear(FAbilityTestSession::GetStartLocation(0), 100.0);
	});
	WaitForDeathMarkBurst(Steps, State, StrikeTime);
	FinishCombo(Steps);
});

// The shadow goes away with its life: once it has, no skill state holds it and no copy is left.
static FScenarioRegistration ZedShadowLifetimeScenario(TEXT("Scenario"), TEXT("Zed.ShadowLifetime"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	TapAtSpot(Steps, Input(TEXT("InputTag.Ability2")), SpotBesideCaster(300.0));
	Steps.WaitUntil(TEXT("A shadow stands on the server"), 3.0, [] { return IsShadowOnServer(); });
	Steps.WaitFor(TEXT("Wait out the shadow's life(5 s) and half a second"), 5.5);

	Steps.Step(TEXT("No skill state holds the shadow"), FAbilityTestSteps::DefaultTimeoutSeconds, [](FString& OutError)
	{
		const int32 OnServer = CountLiveShadowsInSkillState(Caster().ServerChampion.Get());
		const int32 OnClient = CountLiveShadowsInSkillState(Caster().ClientChampion.Get());
		if ((OnServer == 0) && (OnClient == 0))
		{
			return EResult::Next;
		}

		OutError = FString::Printf(TEXT("%d on the server, %d on the owning client"), OnServer, OnClient);
		return EResult::Fail;
	});

	// The shadow plays its way out before it is destroyed.
	Steps.Step(TEXT("No shadow is left on the server or the owning client"), 3.0, [](FString& OutWaitReason)
	{
		const int32 OnServer = CountActors<AAssassinsZedShadow>(FAbilityTestSession::Get().GetServerWorld());
		const int32 OnClient = CountActors<AAssassinsZedShadow>(Caster().ClientWorld.Get());
		if ((OnServer == 0) && (OnClient == 0))
		{
			return EResult::Next;
		}

		OutWaitReason = FString::Printf(TEXT("%d on the server, %d on the owning client"), OnServer, OnClient);
		return EResult::Wait;
	});
});

// R pressed while Q is being cast does not start, and Q goes on and hits.
static FScenarioRegistration ZedRDuringQCastScenario(TEXT("Scenario"), TEXT("Zed.RDuringQCast"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAndPressDuringIt(Steps, TEXT("Q"), Input(TEXT("InputTag.Ability1")), TEXT("R"), Input(TEXT("InputTag.Ability4")));
	CheckDamage(Steps, State, 1.5, AnyDamage);
	FinishCombo(Steps);
});

// Q hits the enemies within the angle of the throw: not one behind Akali.
static FScenarioRegistration AkaliQConeScenario(TEXT("Scenario"), TEXT("Akali.QCone"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Q = Input(TEXT("InputTag.Ability1"));
	const FVector Start = FAbilityTestSession::GetStartLocation(0);
	MovePlayer(Steps, 1, Start + FVector(0.0, 150.0, 0.0));

	CastAbility(Steps, TEXT("Q away from the enemy"), Q, AimAtSpot(Start + FVector(0.0, -300.0, 0.0)));
	Steps.WaitFor(TEXT("Let the kunai land"), 1.0);
	Steps.Step(TEXT("The enemy behind is not hit"), FAbilityTestSteps::DefaultTimeoutSeconds, [State](FString& OutError)
	{
		const float Damage = State->EnemyHealthBefore - State->EnemyLowestHealth;
		OutError = FString::Printf(TEXT("it took %.1f"), Damage);
		return (Damage <= 0.f) ? EResult::Next : EResult::Fail;
	});

	// Pressed again until its cooldown is over.
	CastAbility(Steps, TEXT("Q at the enemy"), Q, AimAtEnemy(), nullptr, 10.0);
	CheckDamage(Steps, State, 1.5, 196.6f);
});

// W: in the shroud's ring Akali is stealthed, out of it she is not(the server decides).
static FScenarioRegistration AkaliWStealthScenario(TEXT("Scenario"), TEXT("Akali.WStealth"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Stealth = FGameplayTag::RequestGameplayTag(TEXT("Status.Champion.Akali.Stealth"));
	const FVector Spot = SpotBesideCaster(280.0);

	CastAbility(Steps, TEXT("W"), Input(TEXT("InputTag.Ability2")), AimAtSpot(Spot));
	Steps.WaitUntil(TEXT("A shroud stands on the server"), 3.0, [] { return CountActors<AAssassinsShroud>(FAbilityTestSession::Get().GetServerWorld()) > 0; });
	WaitForTag(Steps, TEXT("Akali, 280 from the middle, is stealthed in the ring"), 3.0, [] { return CasterOnServer(); }, Stealth);

	Steps.Do(TEXT("Put Akali in the middle of the shroud, out of the ring"), [Spot]
	{
		const AAssassinsShroud* Shroud = FindActor<AAssassinsShroud>(FAbilityTestSession::Get().GetServerWorld());
		const FVector Center = Shroud ? Shroud->GetActorLocation() : Spot;
		Place(Caster().ServerChampion.Get(), Center);
		Place(Caster().ClientChampion.Get(), Center);
	});
	WaitForTag(Steps, TEXT("The stealth ends out of the ring"), 3.0, [] { return CasterOnServer(); }, Stealth, /*bPresent*/ false);
});

// E thrown over the shroud marks it: E again dashes to the mark, with no unit hit.
static FScenarioRegistration AkaliEThroughShroudScenario(TEXT("Scenario"), TEXT("Akali.EThroughShroud"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag E = Input(TEXT("InputTag.Ability3"));
	const FVector Throw = SpotBesideCaster(500.0);
	TSharedRef<FVector> Mark = MakeShared<FVector>(FVector::ZeroVector);

	// Out of the way of the shuriken.
	MovePlayer(Steps, 1, FAbilityTestSession::GetStartLocation(1) + FVector(0.0, 600.0, 0.0));

	CastAbility(Steps, TEXT("W"), Input(TEXT("InputTag.Ability2")), AimAtSpot(SpotBesideCaster(280.0)));
	Steps.WaitUntil(TEXT("A shroud stands on the server"), 3.0, [] { return CountActors<AAssassinsShroud>(FAbilityTestSession::Get().GetServerWorld()) > 0; });
	CastAbility(Steps, TEXT("E over the shroud"), E, AimAtSpot(Throw));

	Steps.Step(TEXT("The shuriken marks the shroud"), 3.0, [Mark](FString& OutWaitReason)
	{
		const AAssassinsShroud* Shroud = FindActor<AAssassinsShroud>(FAbilityTestSession::Get().GetServerWorld());
		if (Shroud && !Shroud->DashMarkLocation.IsNearlyZero())
		{
			*Mark = Shroud->DashMarkLocation;
			return EResult::Next;
		}

		OutWaitReason = Shroud ? TEXT("no mark on the shroud") : TEXT("no shroud");
		return EResult::Wait;
	});

	TapUntil(Steps, TEXT("E again: Akali dashes to the mark"), E, AimAtSpot(Throw), 5.0, [Mark]
	{
		return IsCasterNear(*Mark, 100.0);
	});
	FinishCombo(Steps);
});

// E marks; E again dashes to the marked enemy, and the server deals the damage of the dash(the owning client's end cut it
// off when the client ended first). Then Akali attacks the enemy on her own: the server took that attack(it turned down
// the one the owning client started while the server's dash was still going).
static FScenarioRegistration AkaliERecastScenario(TEXT("Scenario"), TEXT("Akali.ERecastDash"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	TapAtEnemy(Steps, Input(TEXT("InputTag.Ability3")));
	WaitForDamage(Steps, State, 3.0);
	Steps.WaitFor(TEXT("Let the flip end"), 0.5);
	Steps.Do(TEXT("Watch the attack on the server"), [State]
	{
		UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
		State->Recorder = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, Input(TEXT("InputTag.Attack"))));
	});
	TapAtEnemy(Steps, Input(TEXT("InputTag.Ability3")));
	WaitUntilNear(Steps, TEXT("Akali dashes to the enemy"), 3.0, [] { return Caster().ServerChampion.Get(); }, [] { return Enemy().ServerChampion->GetActorLocation(); }, 250.0);
	WaitForEffectFrom(Steps, State, TEXT("E again deals the damage of the dash"), Input(TEXT("InputTag.Ability3")), TEXT("Damage_Dash"));
	Steps.Step(TEXT("Akali goes on attacking the enemy: the attack starts on the server, with no press"), 3.0, [State](FString& OutWaitReason)
	{
		if (State->Recorder->GetNumActivations() > 0)
		{
			return EResult::Next;
		}

		OutWaitReason = State->Recorder->GetFailures().IsEmpty() ? FString(TEXT("no attack on the server")) : State->Recorder->GetFailures();
		return EResult::Wait;
	});
	FinishCombo(Steps);
});

// E again goes after the marked enemy wherever it goes(Migration/ISSUES.md 94): the enemy steps aside once the dash is on
// its way on the server, and Akali ends by it rather than where it stood. Her client sees the step later, and follows it
// later or is brought there by the server: both sides end at the same place.
static FScenarioRegistration AkaliERecastFollowsScenario(TEXT("Scenario"), TEXT("Akali.ERecastFollowsTarget"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector Aside = FAbilityTestSession::GetStartLocation(1) + FVector(300.0, 0.0, 0.0);

	TapAtEnemy(Steps, Input(TEXT("InputTag.Ability3")));
	WaitForDamage(Steps, State, 3.0);
	Steps.WaitFor(TEXT("Let the flip end"), 0.5);
	TapAtEnemy(Steps, Input(TEXT("InputTag.Ability3")));
	WaitForTag(Steps, TEXT("Akali dashes on the server"), 3.0, [] { return Caster().GetServerAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing);
	Steps.Do(TEXT("The enemy steps aside"), [Aside]
	{
		Place(Enemy().ServerChampion.Get(), Aside);
		Place(Enemy().ClientChampion.Get(), Aside);
	});
	WaitForTag(Steps, TEXT("The dash is over on the server"), 3.0, [] { return Caster().GetServerAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	Steps.Step(TEXT("Akali ends by the enemy where it went, on the server"), FAbilityTestSteps::DefaultTimeoutSeconds, [](FString& OutError)
	{
		const AActor* OnServer = Caster().ServerChampion.Get();
		const AActor* Target = Enemy().ServerChampion.Get();
		const double Distance = (OnServer && Target) ? FVector::Dist2D(OnServer->GetActorLocation(), Target->GetActorLocation()) : -1.0;

		// The dash stops 90 short of its target(UAbilityTask_DashToActor).
		if ((Distance < 0.0) || (Distance > 130.0))
		{
			OutError = FString::Printf(TEXT("%.0f from the enemy"), Distance);
			return EResult::Fail;
		}
		return EResult::Next;
	});
	Steps.Step(TEXT("Her client has her at the same place"), 2.0, [](FString& OutWaitReason)
	{
		const AActor* OnServer = Caster().ServerChampion.Get();
		const AActor* OnClient = Caster().ClientChampion.Get();
		const double Apart = (OnServer && OnClient) ? FVector::Dist2D(OnServer->GetActorLocation(), OnClient->GetActorLocation()) : -1.0;
		if ((Apart < 0.0) || (Apart > 30.0))
		{
			OutWaitReason = FString::Printf(TEXT("%.0f apart from the server"), Apart);
			return EResult::Wait;
		}
		return EResult::Next;
	});

	// E ends with the attack it starts once the dash is over: stopped before then, the attack would go on.
	Steps.WaitUntil(TEXT("E is over on the owning client and the server"), 3.0, []
	{
		const FGameplayTag E = Input(TEXT("InputTag.Ability3"));
		const UAbilitySystemComponent* Client = Caster().GetClientAbilitySystem();
		const UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
		const FGameplayAbilitySpec* ClientSpec = Client ? Client->FindAbilitySpecFromClass(FindAbilityClass(Client, E)) : nullptr;
		const FGameplayAbilitySpec* ServerSpec = Server ? Server->FindAbilitySpecFromClass(FindAbilityClass(Server, E)) : nullptr;
		return ClientSpec && ServerSpec && !ClientSpec->IsActive() && !ServerSpec->IsActive();
	});
	FinishCombo(Steps);
});

// R dashes through the enemy; R again starts the recast, its own ability. Both deal damage, and once both are over the
// capsule they find their targets with is off.
static FScenarioRegistration AkaliRRecastScenario(TEXT("Scenario"), TEXT("Akali.RRecast"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));
	Steps.Do(TEXT("Record the recast on the server"), [State]
	{
		State->Recorder = MakeUnique<FAbilityTestRecorder>(Caster().GetServerAbilitySystem(), FindAbilityClass(Caster().GetServerAbilitySystem(), TEXT("Akali_Ability4_Recast")));
	});
	TapAtEnemy(Steps, R);
	WaitForDamage(Steps, State, 3.0);
	TapAtEnemyUntil(Steps, TEXT("The recast activates on the server"), R, 8.0, [State]
	{
		return State->Recorder->GetNumActivations() > 0;
	});
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, R, 328.0f);
	CheckDamageFrom(Steps, State, TEXT("Akali_Ability4_Recast"), 233.6f);

	Steps.Step(TEXT("The ultimate capsule has no collision on the server"), FAbilityTestSteps::DefaultTimeoutSeconds, [](FString& OutError)
	{
		AAssassinsChampion* Champion = Caster().ServerChampion.Get();
		const UAssassinsChampionSkillState_Akali* AkaliState = Champion ? Champion->GetSkillState<UAssassinsChampionSkillState_Akali>() : nullptr;
		const UCapsuleComponent* Capsule = AkaliState ? AkaliState->GetUltimateCapsule() : nullptr;
		if (Capsule == nullptr)
		{
			OutError = TEXT("Akali's skill state has no ultimate capsule");
			return EResult::Fail;
		}

		OutError = FString::Printf(TEXT("its collision is %d"), static_cast<int32>(Capsule->GetCollisionEnabled()));
		return (Capsule->GetCollisionEnabled() == ECollisionEnabled::NoCollision) ? EResult::Next : EResult::Fail;
	});
});

// E pressed while Q is being cast does not start, and Q goes on and hits.
static FScenarioRegistration AkaliEDuringQCastScenario(TEXT("Scenario"), TEXT("Akali.EDuringQCast"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAndPressDuringIt(Steps, TEXT("Q"), Input(TEXT("InputTag.Ability1")), TEXT("E"), Input(TEXT("InputTag.Ability3")));
	CheckDamage(Steps, State, 1.0, 196.6f);
	FinishCombo(Steps);
});

// R again while Q is being cast, once the recast is allowed, does not recast(the recast cancels Q): Q goes on and hits,
// and the recast is still there after the cast.
static FScenarioRegistration AkaliRRecastDuringQCastScenario(TEXT("Scenario"), TEXT("Akali.RRecastDuringQCast"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Q = Input(TEXT("InputTag.Ability1"));
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));

	struct FRecast
	{
		TUniquePtr<FAbilityTestRecorder> OnClient;
		TUniquePtr<FAbilityTestRecorder> OnServer;
	};
	TSharedRef<FRecast> Recast = MakeShared<FRecast>();

	CastAbility(Steps, TEXT("R"), R, AimAtEnemy());
	WaitForDamage(Steps, State, 3.0);
	Steps.WaitFor(TEXT("Wait for the recast to be allowed(2.5 s after the dash)"), 3.0);
	Steps.Do(TEXT("Watch the recast"), [Recast]
	{
		UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
		UAbilitySystemComponent* ServerAbilitySystem = Caster().GetServerAbilitySystem();
		Recast->OnClient = MakeUnique<FAbilityTestRecorder>(ClientAbilitySystem, FindAbilityClass(ClientAbilitySystem, TEXT("Akali_Ability4_Recast")));
		Recast->OnServer = MakeUnique<FAbilityTestRecorder>(ServerAbilitySystem, FindAbilityClass(ServerAbilitySystem, TEXT("Akali_Ability4_Recast")));
	});

	CastAndPressRecastDuringIt(Steps, TEXT("Q"), Q, TEXT("R again"), R, [Recast]
	{
		return (Recast->OnClient->GetNumActivations() > 0) || (Recast->OnServer->GetNumActivations() > 0);
	});
	CheckDamageFrom(Steps, State, Q);

	TapAtEnemyUntil(Steps, TEXT("R again after the cast: the recast activates on the server"), R, 5.0, [Recast] { return Recast->OnServer->GetNumActivations() > 0; });
	FinishCombo(Steps);
});

// R pressed while Q is being cast does not start(R cancels Q when it does), and Q goes on and hits.
static FScenarioRegistration AkaliRDuringQCastScenario(TEXT("Scenario"), TEXT("Akali.RDuringQCast"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAndPressDuringIt(Steps, TEXT("Q"), Input(TEXT("InputTag.Ability1")), TEXT("R"), Input(TEXT("InputTag.Ability4")));
	CheckDamage(Steps, State, 1.0, 196.6f);
	FinishCombo(Steps);
});

// E again while Q is being cast, once the shuriken marked the enemy, does not dash(the dash cut Q short when it did): Q
// goes on and hits, and E again after the cast dashes to the enemy.
static FScenarioRegistration AkaliERecastDuringQCastScenario(TEXT("Scenario"), TEXT("Akali.ERecastDuringQCast"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Q = Input(TEXT("InputTag.Ability1"));
	const FGameplayTag E = Input(TEXT("InputTag.Ability3"));
	const FGameplayTag Dash = FGameplayTag::RequestGameplayTag(TEXT("Status.Combo.Ability3.2"));

	CastAbility(Steps, TEXT("E"), E, AimAtEnemy());
	WaitForDamage(Steps, State, 3.0);

	// On the server too, which flips a little later: a dash moved away from under it never reaches its end.
	const FGameplayTag Flip = FGameplayTag::RequestGameplayTag(TEXT("Status.Combo.Ability3.1"));
	WaitForTag(Steps, TEXT("The flip is over on the owning client"), 3.0, [] { return Caster().GetClientAbilitySystem(); }, Flip, /*bPresent*/ false);
	WaitForTag(Steps, TEXT("The flip is over on the server"), 3.0, [] { return Caster().GetServerAbilitySystem(); }, Flip, /*bPresent*/ false);

	// Back where her Q reaches the enemy: the flip took her 400 away.
	MovePlayer(Steps, 0, FAbilityTestSession::GetStartLocation(0));
	CastAndPressRecastDuringIt(Steps, TEXT("Q"), Q, TEXT("E again"), E, [Dash]
	{
		return HasTag(Caster().GetClientAbilitySystem(), Dash) || HasTag(Caster().GetServerAbilitySystem(), Dash);
	});
	CheckDamageFrom(Steps, State, Q);

	TapUntil(Steps, TEXT("E again after the cast: Akali dashes to the marked enemy"), E, AimAtEnemy(), 5.0, [] { return IsCasterNearEnemy(250.0); });
	WaitForEffectFrom(Steps, State, TEXT("E again deals the damage of the dash"), E, TEXT("Damage_Dash"));

	// E ends with the attack it starts once the dash is over: stopped before then, the attack would go on.
	Steps.WaitUntil(TEXT("E is over on the owning client and the server"), 3.0, [E]
	{
		const UAbilitySystemComponent* Client = Caster().GetClientAbilitySystem();
		const UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
		const FGameplayAbilitySpec* ClientSpec = Client ? Client->FindAbilitySpecFromClass(FindAbilityClass(Client, E)) : nullptr;
		const FGameplayAbilitySpec* ServerSpec = Server ? Server->FindAbilitySpecFromClass(FindAbilityClass(Server, E)) : nullptr;
		return ClientSpec && ServerSpec && !ClientSpec->IsActive() && !ServerSpec->IsActive();
	});
	FinishCombo(Steps);
});

// Q pressed as R starts, and again and again while R waits for the server before its dash and while it dashes, does not
// start(the dash cut it short when it did): R goes on and hits, and Q after the dash hits.
static FScenarioRegistration AkaliQDuringRWaitScenario(TEXT("Scenario"), TEXT("Akali.QDuringRWait"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Q = Input(TEXT("InputTag.Ability1"));
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));

	struct FPresses
	{
		TUniquePtr<FAbilityTestRecorder> ClientR;
		TUniquePtr<FAbilityTestRecorder> ClientQ;
		TUniquePtr<FAbilityTestRecorder> ServerQ;
		TUniquePtr<FAbilityTestRecorder> ServerRecast;
		bool bPressed = false;
		bool bDashSeen = false;

		// The first press of Q came while R waited for the server, before its dash.
		bool bPressedWhileWaiting = false;
	};
	TSharedRef<FPresses> Presses = MakeShared<FPresses>();

	Steps.Do(TEXT("Watch R and Q"), [Presses, Q, R]
	{
		UAbilitySystemComponent* Client = Caster().GetClientAbilitySystem();
		UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
		Presses->ClientR = MakeUnique<FAbilityTestRecorder>(Client, FindAbilityClass(Client, R));
		Presses->ClientQ = MakeUnique<FAbilityTestRecorder>(Client, FindAbilityClass(Client, Q));
		Presses->ServerQ = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, Q));
		Presses->ServerRecast = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, TEXT("Akali_Ability4_Recast")));
	});
	Steps.Do(TEXT("Press R"), [R]
	{
		FAbilityTestDriver::SetAim(Caster(), AimAtEnemy()());
		FAbilityTestDriver::PressInput(Caster(), R);
	});
	Steps.Step(TEXT("Let R go and press Q as R starts"), 3.0, [Presses, Q, R](FString& OutWaitReason)
	{
		if (Presses->ClientR->GetNumActivations() == 0)
		{
			OutWaitReason = FString::Printf(TEXT("R has not started [%s]"), *Presses->ClientR->GetFailures());
			return EResult::Wait;
		}

		FAbilityTestDriver::ReleaseInput(Caster(), R);
		Presses->bPressedWhileWaiting = !HasTag(Caster().GetClientAbilitySystem(), AssassinsGameplayTags::Status_Dashing);
		FAbilityTestDriver::PressInput(Caster(), Q);
		Presses->bPressed = true;
		return EResult::Next;
	});

	Steps.Step(TEXT("Q, pressed again and again until R's dash is over, does not start"), 5.0, [Presses, Q](FString& OutWaitReason)
	{
		if (Presses->bPressed)
		{
			FAbilityTestDriver::ReleaseInput(Caster(), Q);
			Presses->bPressed = false;
		}

		const int32 OnClient = Presses->ClientQ->GetNumActivations();
		const int32 OnServer = Presses->ServerQ->GetNumActivations();
		if ((OnClient > 0) || (OnServer > 0))
		{
			OutWaitReason = FString::Printf(TEXT("Q started before R's dash was over, %d time(s) on the owning client and %d on the server"), OnClient, OnServer);
			return EResult::Fail;
		}

		const bool bDashing = HasTag(Caster().GetClientAbilitySystem(), AssassinsGameplayTags::Status_Dashing);
		Presses->bDashSeen = Presses->bDashSeen || bDashing;
		if (Presses->bDashSeen && !bDashing)
		{
			return EResult::Next;
		}

		FAbilityTestDriver::SetAim(Caster(), AimAtEnemy()());
		FAbilityTestDriver::PressInput(Caster(), Q);
		Presses->bPressed = true;
		OutWaitReason = Presses->bDashSeen ? TEXT("R is still dashing") : TEXT("R still waits for the server");
		return EResult::Wait;
	});

	// A listen server's own player has no one to wait for.
	FAutomationTestBase* Test = &Steps.GetTest();
	Steps.Step(TEXT("Q was pressed while R waited for the server"), FAbilityTestSteps::DefaultTimeoutSeconds, [Test, Presses](FString& OutError)
	{
		if (Presses->bPressedWhileWaiting)
		{
			return EResult::Next;
		}

		if (FAbilityTestSession::Get().GetActiveConfig().bListenServer)
		{
			Test->AddInfo(TEXT("R did not wait before its dash: there was no wait to press Q in"));
			return EResult::Next;
		}

		OutError = TEXT("R already dashed at the first press of Q");
		return EResult::Fail;
	});
	CheckDamageFrom(Steps, State, R);

	CastAbility(Steps, TEXT("Q after the dash"), Q, AimAtEnemy());
	Steps.WaitFor(TEXT("Let Q land"), 1.0);
	CheckDamageFrom(Steps, State, Q);

	// R waits for its recast, and nothing cancels it once it dashed(the reset of the next test included): the recast ends it.
	TapAtEnemyUntil(Steps, TEXT("R again: the recast activates on the server"), R, 8.0, [Presses] { return Presses->ServerRecast->GetNumActivations() > 0; });
	FinishCombo(Steps);
});

// Q, then R: the mimic of Q starts, and R ends as it does.
static FScenarioRegistration LeblancQThenRScenario(TEXT("Scenario"), TEXT("Leblanc.QThenR"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));

	struct FRecorders
	{
		TUniquePtr<FAbilityTestRecorder> ServerR;
		TUniquePtr<FAbilityTestRecorder> ClientR;
		TUniquePtr<FAbilityTestRecorder> ServerMimic;
		TUniquePtr<FAbilityTestRecorder> ClientMimic;
	};
	TSharedRef<FRecorders> Recorders = MakeShared<FRecorders>();

	CastAbility(Steps, TEXT("Q"), Input(TEXT("InputTag.Ability1")), AimAtEnemy());
	Steps.Do(TEXT("Watch R and the mimic of Q"), [Recorders, R]
	{
		UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
		UAbilitySystemComponent* Client = Caster().GetClientAbilitySystem();
		Recorders->ServerR = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, R));
		Recorders->ClientR = MakeUnique<FAbilityTestRecorder>(Client, FindAbilityClass(Client, R));
		Recorders->ServerMimic = MakeUnique<FAbilityTestRecorder>(Server, FindAbilityClass(Server, TEXT("Leblanc_Ability1_Mimic")));
		Recorders->ClientMimic = MakeUnique<FAbilityTestRecorder>(Client, FindAbilityClass(Client, TEXT("Leblanc_Ability1_Mimic")));
	});
	CastAbility(Steps, TEXT("R"), R, AimAtEnemy(), TEXT("Leblanc_Ability1_Mimic"));

	Steps.Step(TEXT("The mimic and R end on the owning client and the server"), 10.0, [Recorders](FString& OutWaitReason)
	{
		const TPair<const TCHAR*, const FAbilityTestRecorder*> All[] = {
			{ TEXT("R on the server"), Recorders->ServerR.Get() }, { TEXT("R on the owning client"), Recorders->ClientR.Get() },
			{ TEXT("the mimic on the server"), Recorders->ServerMimic.Get() }, { TEXT("the mimic on the owning client"), Recorders->ClientMimic.Get() } };

		TArray<FString> Running;
		for (const TPair<const TCHAR*, const FAbilityTestRecorder*>& Pair : All)
		{
			if (Pair.Value->IsActive() || (Pair.Value->GetNumEnds() == 0))
			{
				Running.Add(Pair.Key);
			}
		}
		if (Running.IsEmpty())
		{
			return EResult::Next;
		}

		OutWaitReason = FString::Printf(TEXT("not over: %s"), *FString::Join(Running, TEXT(", ")));
		return EResult::Wait;
	});

	// Where R's recast is handled. The server's R ends when the owning client's does(its end is replicated), which may
	// come before the server's own mimic is done: that gap is reported only.
	FAutomationTestBase* Test = &Steps.GetTest();
	Steps.Step(TEXT("R ends with the mimic on the owning client"), FAbilityTestSteps::DefaultTimeoutSeconds, [Test, Recorders](FString& OutError)
	{
		const double ServerGap = Recorders->ServerR->GetLastEndTime() - Recorders->ServerMimic->GetLastEndTime();
		Test->AddInfo(FString::Printf(TEXT("On the server R ended %.2fs %s the mimic"), FMath::Abs(ServerGap), (ServerGap < 0.0) ? TEXT("before") : TEXT("after")));

		const double Gap = Recorders->ClientR->GetLastEndTime() - Recorders->ClientMimic->GetLastEndTime();
		OutError = FString::Printf(TEXT("R ended %.2fs %s the mimic"), FMath::Abs(Gap), (Gap < 0.0) ? TEXT("before") : TEXT("after"));
		return ((Gap >= -0.05) && (Gap <= 0.5)) ? EResult::Next : EResult::Fail;
	});

	CheckDamageFrom(Steps, State, TEXT("Leblanc_Ability1_Mimic"));
});

// W dashes to a spot; W again brings Leblanc back.
static FScenarioRegistration LeblancWReturnScenario(TEXT("Scenario"), TEXT("Leblanc.WReturn"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector Spot = SpotBesideCaster(300.0);
	const FVector Start = FAbilityTestSession::GetStartLocation(0);
	TapAtSpot(Steps, Input(TEXT("InputTag.Ability2")), Spot);
	WaitUntilNear(Steps, TEXT("Leblanc lands at the spot"), 3.0, [] { return Caster().ServerChampion.Get(); }, [Spot] { return Spot; }, 100.0);

	// One press, once her dash is over on her client: the return listens from then. Close to the spot is not yet over, as
	// the dash ends within 50 of it. A press made the very frame it ended was lost before(Migration/ISSUES.md), see
	// Leblanc.WReturnHeldAtLanding.
	WaitForTag(Steps, TEXT("The dash is over on her client"), 3.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	TapAtSpot(Steps, Input(TEXT("InputTag.Ability2")), Spot);
	WaitUntilNear(Steps, TEXT("W again: Leblanc returns to her start"), 3.0, [] { return Caster().ServerChampion.Get(); }, [Start] { return Start; }, 100.0);
});

// W again pressed during the dash and still down as Leblanc lands returns her as she lands. So does a press made the very
// frame the dash ends, which the game takes before it ends the dash: the wait for the press that cast W to be let go
// took it, and the return needed another press.
static FScenarioRegistration LeblancWReturnHeldAtLandingScenario(TEXT("Scenario"), TEXT("Leblanc.WReturnHeldAtLanding"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag W = Input(TEXT("InputTag.Ability2"));
	const FVector Start = FAbilityTestSession::GetStartLocation(0);

	// As far as W goes(it stops at its range, about 480), for a dash of a few frames more: the press again has to come
	// during it, the frame after the first one is let go. The dash may still be over by then when frames are slow; that
	// is noted.
	const FVector Spot = SpotBesideCaster(600.0);

	struct FHeld
	{
		bool bPressedDuringDash = false;

		// How far from her start the server has had Leblanc: she has to have left before she can be back.
		double FarthestFromStart = 0.0;
	};
	TSharedRef<FHeld> Held = MakeShared<FHeld>();

	TapAtSpot(Steps, W, Spot);
	Steps.Do(TEXT("W again right after, held down"), [W, Spot, Held]
	{
		Held->bPressedDuringDash = HasTag(Caster().GetClientAbilitySystem(), AssassinsGameplayTags::Status_Dashing);
		FAbilityTestDriver::SetAim(Caster(), AimAtSpot(Spot)());
		FAbilityTestDriver::PressInput(Caster(), W);
	});

	// Watched every frame on the server, which may not have moved her yet as the step starts.
	Steps.Step(TEXT("Leblanc dashes away and returns to her start"), 3.0, [Start, Held](FString& OutWaitReason)
	{
		const AActor* Champion = Caster().ServerChampion.Get();
		const double Distance = Champion ? FVector::Dist2D(Champion->GetActorLocation(), Start) : 0.0;
		Held->FarthestFromStart = FMath::Max(Held->FarthestFromStart, Distance);
		if ((Held->FarthestFromStart >= 200.0) && (Distance <= 100.0))
		{
			return EResult::Next;
		}

		OutWaitReason = FString::Printf(TEXT("%.0f from her start, at most %.0f so far"), Distance, Held->FarthestFromStart);
		return EResult::Wait;
	});
	Steps.Do(TEXT("Let W go"), [W] { FAbilityTestDriver::ReleaseInput(Caster(), W); });

	FAutomationTestBase* Test = &Steps.GetTest();
	Steps.Do(TEXT("Note when W was pressed again"), [Test, Held]
	{
		Test->AddInfo(Held->bPressedDuringDash
			? TEXT("W was pressed again during the dash and held as Leblanc landed")
			: TEXT("The dash was over by the second press: only a press after the landing was tested"));
	});
	FinishCombo(Steps);
});

// W to a spot, then R: the mimic of W dashes on to a second spot, and R again brings Leblanc back where the mimic started.
static FScenarioRegistration LeblancWThenRRecastScenario(TEXT("Scenario"), TEXT("Leblanc.WThenRRecast"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));
	const FVector First = SpotBesideCaster(300.0);
	const FVector Second = First + FVector(0.0, -300.0, 0.0);

	// Where both sides have Leblanc at the end, and what runs on each: the 100 ms variant failed twice in 32 runs with no
	// clue in the log(Migration/ISSUES.md 90).
	Steps.Finally([First, Second]
	{
		if (FAbilityTestSession::Get().GetPlayers().Num() == 0)
		{
			return;
		}
		auto Where = [&First, &Second](const AActor* Champion)
		{
			return Champion ? FString::Printf(TEXT("%.0f from first, %.0f from second"), FVector::Dist2D(Champion->GetActorLocation(), First), FVector::Dist2D(Champion->GetActorLocation(), Second)) : FString(TEXT("none"));
		};
		auto Running = [](const UAbilitySystemComponent* AbilitySystem)
		{
			TArray<FString> Names;
			if (AbilitySystem)
			{
				for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
				{
					if (Spec.IsActive() && Spec.Ability)
					{
						Names.Add(GetNameSafe(Spec.Ability->GetClass()));
					}
				}
			}
			return FString::Join(Names, TEXT(", "));
		};
		UE_LOG(LogAssassinsTests, Display, TEXT("Leblanc.WThenRRecast at the end | server: %s, running [%s] | owning client: %s, running [%s]"),
			*Where(Caster().ServerChampion.Get()), *Running(Caster().GetServerAbilitySystem()), *Where(Caster().ClientChampion.Get()), *Running(Caster().GetClientAbilitySystem()));
	});

	CastAbility(Steps, TEXT("W"), Input(TEXT("InputTag.Ability2")), AimAtSpot(First));
	WaitUntilNear(Steps, TEXT("Leblanc lands at the first spot"), 3.0, [] { return Caster().ServerChampion.Get(); }, [First] { return First; }, 100.0);
	CastAbility(Steps, TEXT("R"), R, AimAtSpot(Second), TEXT("Leblanc_Ability2_Mimic"));
	WaitUntilNear(Steps, TEXT("The mimic of W lands at the second spot"), 3.0, [] { return Caster().ServerChampion.Get(); }, [Second] { return Second; }, 100.0);

	// As a player does: the mimic hears the recast only once its dash is over. Leblanc.RRecastDuringMimicDash presses
	// before then.
	WaitForTag(Steps, TEXT("The dash of the mimic is over on the owning client"), 3.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	TapUntil(Steps, TEXT("R again: Leblanc returns where the mimic started"), R, AimAtSpot(Second), 5.0, [First]
	{
		return IsCasterNear(First, 100.0);
	});
	FinishCombo(Steps);
});

// W onto the enemy leaves Leblanc on it, with the avoidance off until they are apart(AAssassinsCharacter::
// WatchPawnOverlapsAfterDash). The mimic of W takes her away before then: the avoidance stays off for that dash as for any,
// though the overlap with the enemy ends on the way, and is on again once she lands clear. Before, the end of that overlap
// turned it on in the middle of the dash.
static FScenarioRegistration LeblancWOntoEnemyScenario(TEXT("Scenario"), TEXT("Leblanc.WOntoEnemyThenMimic"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector Away = SpotBesideEnemy(350.0);
	TSharedRef<FString> AvoidanceDuringDash = MakeShared<FString>();

	CastAbility(Steps, TEXT("W"), Input(TEXT("InputTag.Ability2")), AimAtEnemy());
	WaitUntilNear(Steps, TEXT("Leblanc lands on the enemy"), 3.0, [] { return Caster().ServerChampion.Get(); }, [] { return Enemy().ServerChampion->GetActorLocation(); }, 50.0);
	WaitForTag(Steps, TEXT("The dash is over on the server"), 3.0, [] { return Caster().GetServerAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	WaitForTag(Steps, TEXT("and on the owning client"), 3.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	Steps.Step(TEXT("On the enemy, the avoidance is off on both sides"), 1.0, [](FString& OutWaitReason)
	{
		if (IsAvoiding(Caster().ServerChampion.Get()) || IsAvoiding(Caster().ClientChampion.Get()))
		{
			OutWaitReason = TEXT("the avoidance is on");
			return EResult::Wait;
		}
		return EResult::Next;
	});
	CastAbility(Steps, TEXT("R"), Input(TEXT("InputTag.Ability4")), AimAtSpot(Away), TEXT("Leblanc_Ability2_Mimic"));
	Steps.Step(TEXT("The avoidance stays off while the mimic of W takes her away"), 3.0, [AvoidanceDuringDash, Away](FString& OutError)
	{
		const bool bServerDashing = HasTag(Caster().GetServerAbilitySystem(), AssassinsGameplayTags::Status_Dashing);
		const bool bClientDashing = HasTag(Caster().GetClientAbilitySystem(), AssassinsGameplayTags::Status_Dashing);
		if (bServerDashing && IsAvoiding(Caster().ServerChampion.Get()))
		{
			*AvoidanceDuringDash = TEXT("on the server");
		}
		if (bClientDashing && IsAvoiding(Caster().ClientChampion.Get()))
		{
			*AvoidanceDuringDash = TEXT("on the owning client");
		}
		if (!AvoidanceDuringDash->IsEmpty())
		{
			OutError = FString::Printf(TEXT("the avoidance came on during the dash, %s"), **AvoidanceDuringDash);
			return EResult::Fail;
		}

		if (bServerDashing || bClientDashing || !IsCasterNear(Away, 100.0))
		{
			OutError = TEXT("the dash is not over on both sides");
			return EResult::Wait;
		}
		return EResult::Next;
	});
	Steps.Step(TEXT("Clear of the enemy, the avoidance is on again, and world static blocks her, on both sides"), 1.0, [](FString& OutWaitReason)
	{
		for (const AAssassinsChampion* Champion : { Caster().ServerChampion.Get(), Caster().ClientChampion.Get() })
		{
			if (!IsAvoiding(Champion))
			{
				OutWaitReason = TEXT("the avoidance is off");
				return EResult::Wait;
			}
			if (Champion->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_WorldStatic) != ECR_Block)
			{
				OutWaitReason = TEXT("world static does not block her");
				return EResult::Wait;
			}
		}
		return EResult::Next;
	});
	FinishCombo(Steps);
});

// W lands with Leblanc partly inside a wall, of world static, which a dash goes through: she is pushed out of it as the
// dash ends(AAssassinsCharacter::FinishDashMovement), to the same place on the server and on her client. The wall is put
// up in those two worlds only, and taken away at the end.
static FScenarioRegistration LeblancWAgainstWallScenario(TEXT("Scenario"), TEXT("Leblanc.WLandsAgainstWall"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FVector Landing = SpotBesideCaster(300.0);
	// The near face 30 past the landing. The dash keeps its target(the wall check there is a capsule 20 wide,
	// UAbilityTask_DashTo::AdjustTargetLocation), and Leblanc, 42 wide, ends 12 inside.
	const FVector WallCenter(Landing.X + 30.0 + 50.0, Landing.Y, 150.0);
	TSharedRef<TArray<TWeakObjectPtr<AActor>>> Walls = MakeShared<TArray<TWeakObjectPtr<AActor>>>();

	Steps.Finally([Walls]
	{
		for (const TWeakObjectPtr<AActor>& Wall : *Walls)
		{
			if (Wall.IsValid())
			{
				Wall->Destroy();
			}
		}
	});
	Steps.Do(TEXT("Put up the wall on the server and the owning client"), [Walls, WallCenter]
	{
		TArray<UWorld*> Worlds;
		Worlds.AddUnique(Caster().ServerChampion->GetWorld());
		Worlds.AddUnique(Caster().ClientChampion->GetWorld());
		for (UWorld* World : Worlds)
		{
			Walls->Add(SpawnWall(World, WallCenter, FVector(1.0, 3.0, 3.0)));
		}
	});
	CastAbility(Steps, TEXT("W"), Input(TEXT("InputTag.Ability2")), AimAtSpot(Landing));
	WaitUntilNear(Steps, TEXT("Leblanc lands at the wall"), 3.0, [] { return Caster().ServerChampion.Get(); }, [Landing] { return Landing; }, 50.0);
	WaitForTag(Steps, TEXT("The dash is over on the server"), 3.0, [] { return Caster().GetServerAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	WaitForTag(Steps, TEXT("and on the owning client"), 3.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	Steps.Step(TEXT("Leblanc is out of the wall on both sides, at the same place"), 2.0, [](FString& OutWaitReason)
	{
		const AAssassinsChampion* OnServer = Caster().ServerChampion.Get();
		const AAssassinsChampion* OnClient = Caster().ClientChampion.Get();
		if ((OnServer == nullptr) || (OnClient == nullptr))
		{
			OutWaitReason = TEXT("no champion");
			return EResult::Wait;
		}
		if (IsInsideWorldStatic(OnServer) || IsInsideWorldStatic(OnClient))
		{
			OutWaitReason = IsInsideWorldStatic(OnServer) ? TEXT("inside the wall on the server") : TEXT("inside the wall on the owning client");
			return EResult::Wait;
		}

		const double Apart = FVector::Dist2D(OnServer->GetActorLocation(), OnClient->GetActorLocation());
		if (Apart > 5.0)
		{
			OutWaitReason = FString::Printf(TEXT("%.1f apart"), Apart);
			return EResult::Wait;
		}
		return EResult::Next;
	});
	FinishCombo(Steps);
});

// R again during the dash of the mimic of W does not return her(the mimic can't yet), and R takes the next press: R again
// after the landing brings her back where the mimic started. R took one press only, so the one during the dash left
// her unable to return(Migration/ISSUES.md 86).
static FScenarioRegistration LeblancRRecastDuringMimicDashScenario(TEXT("Scenario"), TEXT("Leblanc.RRecastDuringMimicDash"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));
	const FVector First = SpotBesideCaster(300.0);

	// Farther than the first dash and within the range of W(about 480), for a dash of the mimic of a few frames more: the
	// press again has to come during it.
	const FVector Second = First + FVector(0.0, -450.0, 0.0);
	TSharedRef<bool> bPressedDuringDash = MakeShared<bool>(false);

	CastAbility(Steps, TEXT("W"), Input(TEXT("InputTag.Ability2")), AimAtSpot(First));
	WaitUntilNear(Steps, TEXT("Leblanc lands at the first spot"), 3.0, [] { return Caster().ServerChampion.Get(); }, [First] { return First; }, 100.0);
	WaitForTag(Steps, TEXT("The dash of W is over on the owning client"), 3.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);

	CastAbility(Steps, TEXT("R"), R, AimAtSpot(Second), TEXT("Leblanc_Ability2_Mimic"));
	Steps.Do(TEXT("R again during the dash of the mimic"), [R, Second, bPressedDuringDash]
	{
		*bPressedDuringDash = HasTag(Caster().GetClientAbilitySystem(), AssassinsGameplayTags::Status_Dashing);
		FAbilityTestDriver::SetAim(Caster(), AimAtSpot(Second)());
		FAbilityTestDriver::PressInput(Caster(), R);
	});
	Steps.Do(TEXT("Let R go"), [R] { FAbilityTestDriver::ReleaseInput(Caster(), R); });

	WaitForTag(Steps, TEXT("The dash of the mimic is over on the owning client"), 3.0, [] { return Caster().GetClientAbilitySystem(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	WaitUntilNear(Steps, TEXT("The mimic of W lands at the second spot"), 3.0, [] { return Caster().ServerChampion.Get(); }, [Second] { return Second; }, 100.0);
	TapUntil(Steps, TEXT("R again after the landing: Leblanc returns where the mimic started"), R, AimAtSpot(Second), 5.0, [First]
	{
		return IsCasterNear(First, 100.0);
	});

	FAutomationTestBase* Test = &Steps.GetTest();
	Steps.Do(TEXT("Note when R was pressed again"), [Test, bPressedDuringDash]
	{
		Test->AddInfo(*bPressedDuringDash
			? TEXT("R was pressed again during the dash of the mimic")
			: TEXT("The dash of the mimic was over by the press again: only presses after the landing were tested"));
	});
	FinishCombo(Steps);
});

// The chain lets go of an enemy that gets out of its range, without rooting it.
static FScenarioRegistration LeblancETetherBreakScenario(TEXT("Scenario"), TEXT("Leblanc.ETetherBreak"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Tether = FGameplayTag::RequestGameplayTag(TEXT("Status.Target.Leblanc.Tether.Ability3"));

	CastAbility(Steps, TEXT("E"), Input(TEXT("InputTag.Ability3")), AimAtEnemy());
	WaitForTag(Steps, TEXT("The chain tethers the enemy"), 5.0, [] { return EnemyOnServer(); }, Tether);

	// 1200 from Leblanc, past the tether's 760.
	MovePlayer(Steps, 1, FAbilityTestSession::GetStartLocation(1) + FVector(0.0, 900.0, 0.0));
	WaitForTag(Steps, TEXT("The tether breaks"), 3.0, [] { return EnemyOnServer(); }, Tether, /*bPresent*/ false);

	Steps.WaitFor(TEXT("Wait past the time the tether would have held"), 3.0);
	Steps.Step(TEXT("The enemy is never rooted"), FAbilityTestSteps::DefaultTimeoutSeconds, [State](FString& OutError)
	{
		OutError = TEXT("it was rooted");
		return State->bEnemyRootedSeen ? EResult::Fail : EResult::Next;
	});
});

// From a new session, R does what Q does: the mimic of Q starts. The damage is not held to a number: whether Q's mark
// bursts before R ends depends on timing(198.5 without, 415.4 with).
static FScenarioRegistration LeblancRDefaultScenario(TEXT("Scenario"), TEXT("Leblanc.RDefaultMimic"), [] { return Versus(Leblanc, Zed, /*bFreshSession*/ true); }, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	Steps.Do(TEXT("Record the mimic of Q on the server"), [State]
	{
		State->Recorder = MakeUnique<FAbilityTestRecorder>(Caster().GetServerAbilitySystem(), FindAbilityClass(Caster().GetServerAbilitySystem(), TEXT("Leblanc_Ability1_Mimic")));
	});
	TapAtEnemy(Steps, Input(TEXT("InputTag.Ability4")));
	Steps.Step(TEXT("The mimic of Q activates on the server"), 5.0, [State](FString& OutWaitReason)
	{
		OutWaitReason = State->Recorder->GetFailures();
		return (State->Recorder->GetNumActivations() > 0) ? EResult::Next : EResult::Wait;
	});
	CheckDamage(Steps, State, 2.0, AnyDamage);
});

// W pressed while Q is being cast does not start(it stopped Q before the throw when it did), and Q goes on and hits.
static FScenarioRegistration LeblancWDuringQCastScenario(TEXT("Scenario"), TEXT("Leblanc.WDuringQCast"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	CastAndPressDuringIt(Steps, TEXT("Q"), Input(TEXT("InputTag.Ability1")), TEXT("W"), Input(TEXT("InputTag.Ability2")));
	CheckDamage(Steps, State, 2.0, 163.8f);
	FinishCombo(Steps);
});

// W again while Q is being cast, once Leblanc landed, does not return her(the return cut Q short when it did): Q goes
// on and hits, and W again after the cast returns her.
static FScenarioRegistration LeblancWReturnDuringQCastScenario(TEXT("Scenario"), TEXT("Leblanc.WReturnDuringQCast"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag W = Input(TEXT("InputTag.Ability2"));
	const FVector Spot = SpotBesideCaster(300.0);
	const FVector Start = FAbilityTestSession::GetStartLocation(0);

	CastAbility(Steps, TEXT("W"), W, AimAtSpot(Spot));
	WaitUntilNear(Steps, TEXT("Leblanc lands at the spot"), 3.0, [] { return Caster().ServerChampion.Get(); }, [Spot] { return Spot; }, 100.0);
	CastAndPressRecastDuringIt(Steps, TEXT("Q"), Input(TEXT("InputTag.Ability1")), TEXT("W again"), W, [Start]
	{
		const AActor* ClientChampion = Caster().ClientChampion.Get();
		const bool bReturnedOnClient = ClientChampion && (FVector::Dist2D(ClientChampion->GetActorLocation(), Start) <= 150.0);
		return bReturnedOnClient || IsCasterNear(Start, 150.0);
	});
	CheckDamage(Steps, State, 2.0, 163.8f);

	TapUntil(Steps, TEXT("W again after the cast: Leblanc returns to her start"), W, AimAtSpot(Spot), 3.0, [Start] { return IsCasterNear(Start, 100.0); });
	FinishCombo(Steps);
});

// E pressed the very moment Leblanc lands from W by the enemy, as a quick player presses it: it starts on the server as
// well, and roots the enemy. The server, which hears of the landing with the move that ends the dash, once still had the
// dash going as that press came in with the move, and turned E down(Migration/ISSUES.md 103).
static FScenarioRegistration LeblancEAtWLandingScenario(TEXT("Scenario"), TEXT("Leblanc.EAtWLanding"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag W = Input(TEXT("InputTag.Ability2"));
	const FGameplayTag E = Input(TEXT("InputTag.Ability3"));
	TSharedRef<FPressAtLanding> Landing = MakeShared<FPressAtLanding>();

	Steps.Do(TEXT("E: watch for it, and press it as the dash of W ends on the owning client"), [Landing, E]
	{
		UAbilitySystemComponent* ClientAbilitySystem = Caster().GetClientAbilitySystem();
		UAbilitySystemComponent* ServerAbilitySystem = Caster().GetServerAbilitySystem();
		Landing->OnClient = MakeUnique<FAbilityTestRecorder>(ClientAbilitySystem, FindAbilityClass(ClientAbilitySystem, E));
		Landing->OnServer = MakeUnique<FAbilityTestRecorder>(ServerAbilitySystem, FindAbilityClass(ServerAbilitySystem, E));
		if (ClientAbilitySystem == nullptr)
		{
			return;
		}

		Landing->AbilitySystem = ClientAbilitySystem;
		TWeakPtr<FPressAtLanding> WeakLanding = Landing;
		Landing->Handle = ClientAbilitySystem->RegisterGameplayTagEvent(AssassinsGameplayTags::Status_Dashing, EGameplayTagEventType::NewOrRemoved).AddLambda([WeakLanding, E](const FGameplayTag Tag, int32 NewCount)
		{
			const TSharedPtr<FPressAtLanding> ArmedLanding = WeakLanding.Pin();
			if (ArmedLanding.IsValid() && !ArmedLanding->bPressed && (NewCount == 0))
			{
				FAbilityTestDriver::SetAim(Caster(), AimAtEnemy()());
				FAbilityTestDriver::PressInput(Caster(), E);
				ArmedLanding->bPressed = true;
			}
		});
	});
	Steps.Finally([Landing] { Landing->Disarm(); });

	CastAbility(Steps, TEXT("W"), W, AimAtEnemy());
	Steps.WaitUntil(TEXT("E: pressed as Leblanc landed"), 3.0, [Landing] { return Landing->bPressed; });
	Steps.Do(TEXT("E: let it go"), [Landing, E]
	{
		Landing->Disarm();
		FAbilityTestDriver::ReleaseInput(Caster(), E);
	});

	Steps.Step(TEXT("E starts on the owning client and the server"), 3.0, [Landing](FString& OutWaitReason)
	{
		if ((Landing->OnClient->GetNumActivations() > 0) && (Landing->OnServer->GetNumActivations() > 0))
		{
			return EResult::Next;
		}

		OutWaitReason = FString::Printf(TEXT("%d start(s) on the owning client, %d on the server%s%s"), Landing->OnClient->GetNumActivations(),
			Landing->OnServer->GetNumActivations(), Landing->OnServer->GetFailures().IsEmpty() ? TEXT("") : TEXT(": "), *Landing->OnServer->GetFailures());
		return EResult::Wait;
	});
	Steps.WaitUntil(TEXT("The enemy gets rooted"), 5.0, [State] { return State->bEnemyRootedSeen; });
	FinishCombo(Steps);
});

// R pressed while Q is being cast does not start(it was spent for nothing when it did: the mimic of Q could not start
// during the cast), and Q goes on and hits. R after the cast still does what Q does.
static FScenarioRegistration LeblancRDuringQCastScenario(TEXT("Scenario"), TEXT("Leblanc.RDuringQCast"), Leblanc, Zed, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Q = Input(TEXT("InputTag.Ability1"));
	CastAndPressDuringIt(Steps, TEXT("Q"), Q, TEXT("R"), Input(TEXT("InputTag.Ability4")));
	CheckDamage(Steps, State, 2.0, 163.8f);
	CastAbility(Steps, TEXT("R after the cast"), Input(TEXT("InputTag.Ability4")), AimAtEnemy(), TEXT("Leblanc_Ability1_Mimic"));
	FinishCombo(Steps);
	CheckDamageFrom(Steps, State, Q);
	CheckDamageFrom(Steps, State, TEXT("Leblanc_Ability1_Mimic"));
});

// One press, and the attack goes on by itself(GA_ActivateAttack): a second hit comes.
static FScenarioRegistration AttackChainScenario(TEXT("Scenario"), TEXT("Attack.Chain"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	AttackOnce(Steps, State);
	Steps.Step(TEXT("The attack goes on: a second hit"), 8.0, [State](FString& OutWaitReason)
	{
		const FString Attack = GetNameSafe(FindAbilityClass(Caster().GetServerAbilitySystem(), Input(TEXT("InputTag.Attack"))));
		const int32 Hits = State->DamageLog->CountHits(Attack);
		OutWaitReason = FString::Printf(TEXT("%d hit(s) so far"), Hits);
		return (Hits >= 2) ? EResult::Next : EResult::Wait;
	});
	FinishCombo(Steps);
});

// Attacking one enemy, a click on another takes the attack to it. Four players: the teams fill the one with fewer
// players first, so the fourth is an enemy too.
static FScenarioRegistration AttackRetargetScenario(TEXT("Scenario"), TEXT("Attack.Retarget"), []
{
	FAbilityTestSessionConfig Config;
	Config.Champions = { FSoftObjectPath(Zed), FSoftObjectPath(Akali), FSoftObjectPath(Leblanc), FSoftObjectPath(Akali) };
	return Config;
}, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag AttackInput = Input(TEXT("InputTag.Attack"));

	struct FRetarget
	{
		TUniquePtr<FAbilityTestDamageLog> OtherDamageLog;
		FString AttackClassName;
		double SwitchTime = 0.0;
	};
	TSharedRef<FRetarget> Retarget = MakeShared<FRetarget>();

	Steps.Step(TEXT("The fourth player is an enemy too"), FAbilityTestSteps::DefaultTimeoutSeconds, [](FString& OutError)
	{
		const AAssassinsPlayerState* CasterState = Caster().ServerController.IsValid() ? Caster().ServerController->GetPlayerState<AAssassinsPlayerState>() : nullptr;
		const AAssassinsPlayerState* OtherState = PlayerAt(OtherEnemy).ServerController.IsValid() ? PlayerAt(OtherEnemy).ServerController->GetPlayerState<AAssassinsPlayerState>() : nullptr;
		const int32 CasterTeam = CasterState ? GenericTeamIdToInteger(CasterState->GetGenericTeamId()) : INDEX_NONE;
		const int32 OtherTeam = OtherState ? GenericTeamIdToInteger(OtherState->GetGenericTeamId()) : INDEX_NONE;
		OutError = FString::Printf(TEXT("the caster is on team %d, the fourth player on team %d"), CasterTeam, OtherTeam);
		return ((CasterTeam != INDEX_NONE) && (OtherTeam != INDEX_NONE) && (CasterTeam != OtherTeam)) ? EResult::Next : EResult::Fail;
	});

	MovePlayer(Steps, OtherEnemy, SpotBesideCaster(300.0));
	Steps.Do(TEXT("Watch what the other enemy takes"), [Retarget, AttackInput]
	{
		Retarget->OtherDamageLog = MakeUnique<FAbilityTestDamageLog>(PlayerAt(OtherEnemy).GetServerAbilitySystem());
		Retarget->AttackClassName = GetNameSafe(FindAbilityClass(Caster().GetServerAbilitySystem(), AttackInput));
	});

	AttackOnce(Steps, State);

	// One click, as a player does: every press starts the attack over.
	Steps.Do(TEXT("Aim at the other enemy"), []
	{
		FAbilityTestDriver::SetAim(Caster(), AimAtPlayer(OtherEnemy)());
	});
	FAbilityTestDriver::AddTapSteps(Steps, 0, AttackInput);
	Steps.Step(TEXT("The attack hits the other enemy"), 8.0, [Retarget](FString& OutWaitReason)
	{
		if (Retarget->OtherDamageLog->CountHits(Retarget->AttackClassName) > 0)
		{
			Retarget->SwitchTime = FPlatformTime::Seconds();
			return EResult::Next;
		}

		OutWaitReason = TEXT("no hit on it yet");
		return EResult::Wait;
	});

	Steps.WaitFor(TEXT("Keep attacking for a while"), 2.0);
	Steps.Step(TEXT("The first enemy is not hit anymore"), FAbilityTestSteps::DefaultTimeoutSeconds, [State, Retarget](FString& OutError)
	{
		const int32 Hits = State->DamageLog->CountHits(Retarget->AttackClassName, Retarget->SwitchTime);
		OutError = FString::Printf(TEXT("hit %d more time(s) after the switch"), Hits);
		return (Hits == 0) ? EResult::Next : EResult::Fail;
	});

	FinishCombo(Steps);
	FAutomationTestBase* Test = &Steps.GetTest();
	Steps.Finally([Test, Retarget]
	{
		if (Retarget->OtherDamageLog)
		{
			Test->AddInfo(FString::Printf(TEXT("Damage to the other enemy by ability: %s"), *Retarget->OtherDamageLog->DescribeByAbility()));
			UE_LOG(LogAssassinsTests, Display, TEXT("Damage log of the other enemy | %s\n%s"), *Test->GetTestFullName(), *Retarget->OtherDamageLog->Describe());
		}
		Retarget->OtherDamageLog.Reset();
	});
});

// A blow from the caster gives it the aggro of attacking a champion, which turrets and minions go by; a heal from it does
// not(Migration/ISSUES.md 55). All on the server: no variant.
static FScenarioRegistration ChampionAggroScenario(TEXT("Scenario"), TEXT("Champion.AggroFromDamageOnly"), Zed, Akali, /*bWithVariants*/ false, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Aggro = FGameplayTag::RequestGameplayTag(TEXT("Status.Aggro.ChampionAttack"));

	// Room for the heal, from no champion.
	Steps.Do(TEXT("The enemy loses some health"), [] { ChangeHealthOnServer(nullptr, Enemy(), -100.f); });
	Steps.Do(TEXT("The caster heals the enemy"), [] { ChangeHealthOnServer(&Caster(), Enemy(), 50.f); });
	Steps.Step(TEXT("The heal lands and draws no aggro to the caster"), FAbilityTestSteps::DefaultTimeoutSeconds, [State, Aggro](FString& OutError)
	{
		const float Health = EnemyHealth();
		if (FMath::Abs(Health - (State->EnemyHealthBefore - 50.f)) > 1.f)
		{
			OutError = FString::Printf(TEXT("the enemy has %.1f, %.1f before"), Health, State->EnemyHealthBefore);
			return EResult::Fail;
		}

		OutError = TEXT("the caster has the aggro");
		return HasTag(CasterOnServer(), Aggro) ? EResult::Fail : EResult::Next;
	});

	Steps.Do(TEXT("The caster deals the enemy a blow"), [] { ChangeHealthOnServer(&Caster(), Enemy(), -50.f); });
	WaitForTag(Steps, TEXT("The blow draws aggro to the caster"), 1.0, [] { return CasterOnServer(); }, Aggro);
});

// The same for a minion: a blow from the caster gives it the aggro of attacking a minion, which the minions go by; a heal
// does not(Migration/ISSUES.md 98). All on the server: no variant.
static FScenarioRegistration MinionAggroScenario(TEXT("Scenario"), TEXT("Minion.AggroFromDamageOnly"), Zed, Akali, /*bWithVariants*/ false, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag Aggro = FGameplayTag::RequestGameplayTag(TEXT("Status.Aggro.MinionAttack"));

	struct FMinion
	{
		TWeakObjectPtr<AActor> Actor;
		float HealthBefore = 0.f;

		UAbilitySystemComponent* GetAbilitySystem() const
		{
			return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor.Get());
		}
	};
	TSharedRef<FMinion> Minion = MakeShared<FMinion>();

	Steps.Step(TEXT("A minion stands aside on the server"), FAbilityTestSteps::DefaultTimeoutSeconds, [Minion](FString& OutError)
	{
		Minion->Actor = SpawnMinionOnServer(SpotBesideCaster(-600.0));
		Minion->HealthBefore = AbilityTestChecks::GetHealth(Minion->GetAbilitySystem());
		OutError = TEXT("no minion with an ability system");
		return (Minion->GetAbilitySystem() && (Minion->HealthBefore > 100.f)) ? EResult::Next : EResult::Fail;
	});
	Steps.Finally([Minion]
	{
		if (AActor* Spawned = Minion->Actor.Get())
		{
			Spawned->Destroy();
		}
	});

	// Room for the heal, from no player.
	Steps.Do(TEXT("The minion loses some health"), [Minion] { ChangeHealthOnServer(nullptr, Minion->GetAbilitySystem(), -100.f); });
	Steps.Do(TEXT("The caster heals the minion"), [Minion] { ChangeHealthOnServer(&Caster(), Minion->GetAbilitySystem(), 50.f); });
	Steps.Step(TEXT("The heal lands and draws no aggro to the caster"), FAbilityTestSteps::DefaultTimeoutSeconds, [Minion, Aggro](FString& OutError)
	{
		const float Health = AbilityTestChecks::GetHealth(Minion->GetAbilitySystem());
		if (FMath::Abs(Health - (Minion->HealthBefore - 50.f)) > 1.f)
		{
			OutError = FString::Printf(TEXT("the minion has %.1f, %.1f before"), Health, Minion->HealthBefore);
			return EResult::Fail;
		}

		OutError = TEXT("the caster has the aggro");
		return HasTag(CasterOnServer(), Aggro) ? EResult::Fail : EResult::Next;
	});

	Steps.Do(TEXT("The caster deals the minion a blow"), [Minion] { ChangeHealthOnServer(&Caster(), Minion->GetAbilitySystem(), -50.f); });
	WaitForTag(Steps, TEXT("The blow draws aggro to the caster"), 1.0, [] { return CasterOnServer(); }, Aggro);
});

// In the scenarios where the target dies, the enemy does not come back: the test after one brings up a new session.

// R at the enemy, which dies while Zed casts(Migration/ISSUES.md 23). Its death plays longer than the cast, so it is still
// there as the cast ends: Zed reappears behind where it stood, not toward the middle of the map, R again takes him back
// to his shadow, and R ends on both sides, with Zed in the world again.
static FScenarioRegistration ZedRTargetDiesScenario(TEXT("Scenario"), TEXT("Zed.RTargetDiesDuringCast"), Zed, Akali, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag R = Input(TEXT("InputTag.Ability4"));
	const FVector Start = FAbilityTestSession::GetStartLocation(0);
	const FVector EnemyStart = FAbilityTestSession::GetStartLocation(1);

	// 150 past the target, on the line from Zed through it(UGA_Zed_Ability4::TeleportToTarget).
	const FVector Behind = EnemyStart + (EnemyStart - Start).GetSafeNormal2D() * 150.0;

	CastAbility(Steps, TEXT("R"), R, AimAtEnemy());
	WaitForTag(Steps, TEXT("Zed casts on the server"), 3.0, [] { return CasterOnServer(); }, AssassinsGameplayTags::Status_Channeling);
	KillEnemy(Steps);
	WaitUntilNear(Steps, TEXT("Zed reappears behind where the enemy stood, on the server"), 4.0, [] { return Caster().ServerChampion.Get(); }, [Behind] { return Behind; }, 100.0);
	TapUntil(Steps, TEXT("R again: Zed goes back to his shadow"), R, AimAtSpot(Start), 5.0, [Start]
	{
		return IsCasterNear(Start, 100.0);
	});
	FinishCombo(Steps);
	WaitForEnemyToGo(Steps);

	Steps.Step(TEXT("Zed is in the world again, on the server and the owning client"), FAbilityTestSteps::DefaultTimeoutSeconds, [](FString& OutError)
	{
		for (const AAssassinsChampion* Champion : { Caster().ServerChampion.Get(), Caster().ClientChampion.Get() })
		{
			const UCapsuleComponent* Capsule = Champion ? Champion->GetCapsuleComponent() : nullptr;
			if ((Capsule == nullptr) || (Capsule->GetCollisionEnabled() != ECollisionEnabled::QueryOnly))
			{
				OutError = FString::Printf(TEXT("the capsule of %s has collision %d"), *GetNameSafe(Champion), Capsule ? static_cast<int32>(Capsule->GetCollisionEnabled()) : -1);
				return EResult::Fail;
			}
		}
		return EResult::Next;
	});
});

// E marks the enemy; E again dashes to it, and it dies on the way(Migration/ISSUES.md 24): the dash and E still end on
// both sides, and its death plays out to its end.
static FScenarioRegistration AkaliETargetDiesScenario(TEXT("Scenario"), TEXT("Akali.ETargetDiesDuringDash"), Akali, Leblanc, [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
{
	const FGameplayTag E = Input(TEXT("InputTag.Ability3"));

	TapAtEnemy(Steps, E);
	WaitForDamage(Steps, State, 3.0);
	Steps.WaitFor(TEXT("Let the flip end"), 0.5);
	TapAtEnemy(Steps, E);
	WaitForTag(Steps, TEXT("Akali dashes on the server"), 3.0, [] { return CasterOnServer(); }, AssassinsGameplayTags::Status_Dashing);
	KillEnemy(Steps);
	WaitForTag(Steps, TEXT("The dash is over on the server"), 3.0, [] { return CasterOnServer(); }, AssassinsGameplayTags::Status_Dashing, /*bPresent*/ false);
	Steps.WaitUntil(TEXT("E is over on the owning client and the server"), 5.0, [E]
	{
		const UAbilitySystemComponent* Client = Caster().GetClientAbilitySystem();
		const UAbilitySystemComponent* Server = Caster().GetServerAbilitySystem();
		const FGameplayAbilitySpec* ClientSpec = Client ? Client->FindAbilitySpecFromClass(FindAbilityClass(Client, E)) : nullptr;
		const FGameplayAbilitySpec* ServerSpec = Server ? Server->FindAbilitySpecFromClass(FindAbilityClass(Server, E)) : nullptr;
		return ClientSpec && ServerSpec && !ClientSpec->IsActive() && !ServerSpec->IsActive();
	});
	WaitForEnemyToGo(Steps);
	FinishCombo(Steps);
});

// The scenarios above, and again with lag on the clients and with the caster hosting a listen server. The variants are
// named apart from Scenario, so that running Scenario doesn't run them too.
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FScenarioTests, "Assassins.Abilities.Scenario", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FScenarioLag100Tests, "Assassins.Abilities.Lag100.Scenario", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FScenarioLag200Tests, "Assassins.Abilities.Lag200.Scenario", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FScenarioListenTests, "Assassins.Abilities.Listen.Scenario", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FScenarioTests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Scenario"), OutBeautifiedNames, OutTestCommands);
}

bool FScenarioTests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Scenario"), Parameters, /*bListenServer*/ false, /*LagMs*/ 0);
}

void FScenarioLag100Tests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Scenario"), OutBeautifiedNames, OutTestCommands, /*bVariant*/ true);
}

bool FScenarioLag100Tests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Scenario"), Parameters, /*bListenServer*/ false, /*LagMs*/ 100);
}

void FScenarioLag200Tests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Scenario"), OutBeautifiedNames, OutTestCommands, /*bVariant*/ true);
}

bool FScenarioLag200Tests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Scenario"), Parameters, /*bListenServer*/ false, /*LagMs*/ 200);
}

void FScenarioListenTests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	GetScenarioTests(TEXT("Scenario"), OutBeautifiedNames, OutTestCommands, /*bVariant*/ true);
}

bool FScenarioListenTests::RunTest(const FString& Parameters)
{
	return RunScenarioTest(*this, TEXT("Scenario"), Parameters, /*bListenServer*/ true, /*LagMs*/ 0);
}

// The damage check has to fail on a wrong expectation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelfCheckWrongDamage, "Assassins.Abilities.SelfCheck.WrongDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSelfCheckWrongDamage::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("expected damage 1000000"), EAutomationExpectedErrorFlags::Contains, 1);
	return Run(*this, Versus(Zed, Akali), [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
	{
		TapAtEnemy(Steps, Input(TEXT("InputTag.Ability1")));
		CheckDamage(Steps, State, 1.5, 1000000.f);
	});
}

// Waiting for an ability that never starts has to time out.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSelfCheckNoActivation, "Assassins.Abilities.SelfCheck.NoActivation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSelfCheckNoActivation::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("Q activates on the server: timed out"), EAutomationExpectedErrorFlags::Contains, 1);
	return Run(*this, Versus(Zed, Akali), [](FAbilityTestSteps& Steps, TSharedRef<FState> State)
	{
		Steps.Do(TEXT("Record Q on the server"), [State]
		{
			State->Recorder = MakeUnique<FAbilityTestRecorder>(Caster().GetServerAbilitySystem(), FindAbilityClass(Caster().GetServerAbilitySystem(), Input(TEXT("InputTag.Ability1"))));
		});
		TapAtEnemy(Steps, Input(TEXT("InputTag.Ability2")));
		Steps.WaitUntil(TEXT("Q activates on the server"), 2.0, [State] { return State->Recorder->GetNumActivations() > 0; });
	});
}

} // namespace AbilityScenarioTests

#endif // WITH_DEV_AUTOMATION_TESTS
