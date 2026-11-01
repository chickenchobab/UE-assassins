#include "AbilitySystem/AssassinsInterframeHitCheckSubsystem.h"

#include "AssassinsLogCategories.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Stats/Stats.h"

DECLARE_CYCLE_STAT(TEXT("Interframe Hit Check"), STAT_AssassinsInterframeHitCheck, STATGROUP_Game);

namespace AssassinsInterframeHitCheck
{
	static int32 Mode = 1;
	static FAutoConsoleVariableRef CVarMode(
		TEXT("Assassins.InterframeHitCheck.Mode"),
		Mode,
		TEXT("How a dash finds the characters it passes through.\n")
		TEXT("0: sweeps the dash's shape alone against where everyone stands.\n")
		TEXT("1: checks the dash against the server's record of everyone over the same frame, as the frame ends."),
		ECVF_Default);

	// A를 정지시킨 A 기준 좌표에서 B는 두 이동의 차이만큼 움직인다. 그러면 지면 거리는 시간의 이차식이고, 처음 반지름 합까지
	// 줄어드는 시각이 충돌 시각이다. 높이는 그 시각에 본다. 세운 캡슐은 반지름만큼 부푼 수직 선분이라, 두 선분 사이 거리가
	// 반지름 합 안이면 닿는다.
	FAssassinsSweptPairResult SolvePair(const FAssassinsSweptPair& Pair)
	{
		FAssassinsSweptPairResult Result;

		// A 기준 지면 좌표.
		const FVector StartDelta = Pair.StartB - Pair.StartA;
		const FVector MoveDelta = (Pair.EndB - Pair.StartB) - (Pair.EndA - Pair.StartA);
		const FVector2D StartOffset(StartDelta.X, StartDelta.Y);
		const FVector2D RelativeMove(MoveDelta.X, MoveDelta.Y);
		const double Reach = static_cast<double>(Pair.RadiusA) + static_cast<double>(Pair.RadiusB);

		// |StartOffset + RelativeMove * t|^2 = Reach^2
		const double A = RelativeMove.SizeSquared();
		const double B = 2.0 * FVector2D::DotProduct(StartOffset, RelativeMove);
		const double C = StartOffset.SizeSquared() - Reach * Reach;

		// 구간 안에서 제곱 거리가 줄다 멈추는 곳이 가장 가깝다. 서로 정지해 있으면 시작점이다.
		Result.ClosestTime = (A > UE_SMALL_NUMBER) ? FMath::Clamp(-B / (2.0 * A), 0.0, 1.0) : 0.0;
		Result.ClosestDistance = (StartOffset + RelativeMove * Result.ClosestTime).Size();

		double GroundTouchTime = -1.0;
		if (C <= 0.0)
		{
			GroundTouchTime = 0.0;
		}
		else if (A > UE_SMALL_NUMBER)
		{
			const double Discriminant = B * B - 4.0 * A * C;
			if (Discriminant >= 0.0)
			{
				// 거리가 처음 닿는 거리로 줄어드는 더 이른 근. C가 0보다 크면 두 근의 부호가 같아, 근이 0보다 작으면 둘은 멀어지기만 한다.
				const double Root = (-B - FMath::Sqrt(Discriminant)) / (2.0 * A);
				if ((Root >= 0.0) && (Root <= 1.0))
				{
					GroundTouchTime = Root;
				}
			}
		}

		if (GroundTouchTime < 0.0)
		{
			return Result;
		}

		const double SegmentHalfA = FMath::Max(static_cast<double>(Pair.HalfHeightA) - Pair.RadiusA, 0.0);
		const double SegmentHalfB = FMath::Max(static_cast<double>(Pair.HalfHeightB) - Pair.RadiusB, 0.0);
		auto TouchesAt = [&](double Time)
		{
			const double HeightA = FMath::Lerp(Pair.StartA.Z, Pair.EndA.Z, Time);
			const double HeightB = FMath::Lerp(Pair.StartB.Z, Pair.EndB.Z, Time);
			const double HeightGap = FMath::Max(FMath::Abs(HeightB - HeightA) - SegmentHalfA - SegmentHalfB, 0.0);
			const double GroundDistanceSquared = (StartOffset + RelativeMove * Time).SizeSquared();
			return (GroundDistanceSquared + HeightGap * HeightGap) <= (Reach * Reach + UE_KINDA_SMALL_NUMBER);
		};

		if (TouchesAt(GroundTouchTime))
		{
			Result.bHit = true;
			Result.TimeOfImpact = GroundTouchTime;
		}
		else if (TouchesAt(Result.ClosestTime))
		{
			// 지면에서 처음 만날 때는 높이가 떨어져 있지만 가장 가까울 때 닿으면, 닿은 시각을 처음이 아닌 그때로 둔다.
			Result.bHit = true;
			Result.TimeOfImpact = Result.ClosestTime;
		}
		return Result;
	}
}

FVector UAssassinsInterframeHitCheckSubsystem::FCharacterRecord::LocationAt(double Time) const
{
	// 판정 구간은 직전 기록과 현재 기록 사이라, 둘 사이를 보간한다.
	const double Span = Current.Time - Previous.Time;
	const double Alpha = (Span > 0.0) ? FMath::Clamp((Time - Previous.Time) / Span, 0.0, 1.0) : 1.0;
	return FMath::Lerp(Previous.Location, Current.Location, Alpha);
}

// 돌진을 판정할 때 대상 캐릭터의 지금 위치가 아니라, 같은 시간 구간 동안 있던 위치와 비교한다. 돌진만 쓸어 보면 대상을 한
// 지점에서만 읽어서, 대상도 움직이면 두 이동 사이에 스쳐 지나가는 것을 놓친다.
// 서버는 프레임이 끝날 때마다 모든 캐릭터의 위치를 그 프레임의 서버 시각으로 기록한다(직전 기록과 현재 기록). 늦게 온 이동은
// 도착한 프레임에 기록되므로 지난 구간에 끼어들지 않는다. 돌진(BeginDashHitShape), 투사체(CheckProjectileSpanHit), 순간
// 판정(CheckHitAt)은 프레임 중에 쌓였다가 그 프레임 기록 뒤에 같은 서버 시간 구간의 기록과 판정한다. 판정은 서버 전용이다.
UAssassinsInterframeHitCheckSubsystem* UAssassinsInterframeHitCheckSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UAssassinsInterframeHitCheckSubsystem>() : nullptr;
}

bool UAssassinsInterframeHitCheckSubsystem::UsesServerRecords()
{
	return AssassinsInterframeHitCheck::Mode != 0;
}

void UAssassinsInterframeHitCheckSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &ThisClass::HandlePostActorTick);
}

void UAssassinsInterframeHitCheckSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldPostActorTick.Remove(PostActorTickHandle);
	PostActorTickHandle.Reset();

	Super::Deinitialize();
}

bool UAssassinsInterframeHitCheckSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return (WorldType == EWorldType::Game) || (WorldType == EWorldType::PIE);
}

void UAssassinsInterframeHitCheckSubsystem::Register(ACharacter* Character)
{
	if ((Character == nullptr) || !IsServer())
	{
		return;
	}

	FCharacterRecord& Record = CharacterRecords.FindOrAdd(TObjectKey<ACharacter>(Character));
	Record = FCharacterRecord();
	Record.Character = Character;

	// 시작할 때 선 위치(첫 프레임 기록 전 판정용).
	const UWorld* World = GetWorld();
	Record.Current.Time = World ? World->GetTimeSeconds() : 0.0;
	Record.Current.Location = Character->GetActorLocation();
	Record.Previous = Record.Current;
}

void UAssassinsInterframeHitCheckSubsystem::Unregister(ACharacter* Character)
{
	CharacterRecords.Remove(TObjectKey<ACharacter>(Character));
	PendingHitChecks.RemoveAll([Character](const FPendingHitCheck& Each)
	{
		return (Each.Check.Mover.Get() == Character) || !Each.Check.Mover.IsValid();
	});
	DashHitShapes.RemoveAll([Character](const FDashHitShape& HitShape)
	{
		return (HitShape.Dasher.Get() == Character) || !HitShape.Dasher.IsValid();
	});
}

void UAssassinsInterframeHitCheckSubsystem::OnServerCharacterMoved(ACharacter* Character, const FVector& OldLocation)
{
	if ((Character == nullptr) || !IsServer() || !CharacterRecords.Contains(TObjectKey<ACharacter>(Character)))
	{
		return;
	}

	// 돌진은 원격 플레이어든 서버가 움직이는 캐릭터든 프레임 기록 뒤에 같은 서버 구간으로 판정한다(RunPendingHitChecks).
	// 옛 스윕은 이동마다 바로 한다.
	CheckDashMove(Character, OldLocation, Character->GetActorLocation());
}

void UAssassinsInterframeHitCheckSubsystem::RecordTeleport(ACharacter* Character)
{
	FCharacterRecord* Record = ((Character != nullptr) && IsServer()) ? CharacterRecords.Find(TObjectKey<ACharacter>(Character)) : nullptr;
	if (Record != nullptr)
	{
		Record->bTeleportRecorded = true;
	}
}

void UAssassinsInterframeHitCheckSubsystem::RecordPositions()
{
	const UWorld* World = GetWorld();
	if ((World == nullptr) || !IsServer())
	{
		return;
	}

	// 모든 액터가 틱한 뒤라, 이 위치가 이번 프레임에 서버가 처리한 이동의 결과다.
	const double Now = World->GetTimeSeconds();
	for (TPair<TObjectKey<ACharacter>, FCharacterRecord>& Entry : CharacterRecords)
	{
		FCharacterRecord& Record = Entry.Value;
		const ACharacter* Character = Record.Character.Get();
		if (Character == nullptr)
		{
			continue;
		}

		// 같은 프레임에 등록과 프레임 기록이 겹치면 현재 기록을 그 프레임의 위치로 바꾼다.
		if (Now > Record.Current.Time)
		{
			Record.Previous = Record.Current;
			Record.Current.bTeleported = false;
		}
		Record.Current.Time = Now;
		Record.Current.Location = Character->GetActorLocation();
		Record.Current.bTeleported = Record.Current.bTeleported || Record.bTeleportRecorded;
		Record.bTeleportRecorded = false;
	}
	PreviousRecordedTime = (LatestRecordedTime >= 0.0) ? LatestRecordedTime : Now;
	LatestRecordedTime = Now;
}

void UAssassinsInterframeHitCheckSubsystem::RunPendingHitChecks()
{
	if (!IsServer())
	{
		return;
	}

	// 프레임 중에 쌓인 판정(돌진, 투사체 구간, 순간 판정). 이번 프레임 기록까지 끝난 뒤라 모든 대상의 같은 서버 시간 구간과
	// 판정하고, 명중은 한데 모아 시각 순으로 넘긴 뒤 판정 끝을 알린다.
	TArray<FHitCheck, TInlineAllocator<4>> Hits;
	TArray<FPendingHitCheck> Pending = MoveTemp(PendingHitChecks);
	PendingHitChecks.Reset();
	for (FPendingHitCheck& Each : Pending)
	{
		if (!Each.Check.Mover.IsValid())
		{
			continue;
		}

		// 원격 이동은 프레임 시각이 오르기 전에 처리되므로, 돌진과 순간 판정의 구간은 쌓을 때가 아니라 지금 정한다.
		if (Each.Span != EPendingSpan::Given)
		{
			Each.Check.TargetFromTime = (Each.Span == EPendingSpan::Frame) ? PreviousRecordedTime : LatestRecordedTime;
			Each.Check.TargetToTime = LatestRecordedTime;
			Each.Check.MoverFromTime = Each.Check.TargetFromTime;
			Each.Check.MoverToTime = Each.Check.TargetToTime;
		}
		CheckHitAgainstAllCharacters(Each.Check, Hits);
	}

	DeliverHits(Hits);

	for (FPendingHitCheck& Each : Pending)
	{
		if (Each.OnDone)
		{
			Each.OnDone();
		}
	}
}

int32 UAssassinsInterframeHitCheckSubsystem::BeginDashHitShape(ACharacter* Dasher, const FAssassinsInterframeHitShape& Shape, FCanHit CanHit, FOnHit OnHit)
{
	if ((Dasher == nullptr) || !CanHit || !OnHit || !IsServer())
	{
		return 0;
	}

	FDashHitShape& HitShape = DashHitShapes.AddDefaulted_GetRef();
	HitShape.Handle = ++LastHandle;
	HitShape.Dasher = Dasher;
	HitShape.Shape = Shape;
	HitShape.CanHit = MoveTemp(CanHit);
	HitShape.OnHit = MoveTemp(OnHit);
	return HitShape.Handle;
}

void UAssassinsInterframeHitCheckSubsystem::EndDashHitShape(int32 Handle)
{
	// 모양이 켜져 있을 때 판정에 넘긴 이동은 프레임 끝으로 미룬 것까지 그대로 판정한다.
	DashHitShapes.RemoveAll([Handle](const FDashHitShape& HitShape) { return HitShape.Handle == Handle; });
}

void UAssassinsInterframeHitCheckSubsystem::CheckHitAt(ACharacter* Mover, const FVector& Location, const FAssassinsInterframeHitShape& Shape, FCanHit CanHit, FOnHitsChecked OnHitsChecked)
{
	SCOPE_CYCLE_COUNTER(STAT_AssassinsInterframeHitCheck);

	if ((Mover == nullptr) || !CanHit || !OnHitsChecked || !IsServer())
	{
		return;
	}

	// 맞은 대상을 모았다가 판정이 끝나면 한 번에 넘긴다. 명중이 없어도 넘긴다.
	TSharedRef<TArray<TWeakObjectPtr<AActor>>> Collected = MakeShared<TArray<TWeakObjectPtr<AActor>>>();
	FOnHit Collect = [Collected](AActor* Target) { Collected->Add(Target); };
	TFunction<void()> Finish = [Collected, OnHitsChecked = MoveTemp(OnHitsChecked)]()
	{
		TArray<AActor*> Hits;
		for (const TWeakObjectPtr<AActor>& Target : *Collected)
		{
			if (AActor* Actor = Target.Get())
			{
				Hits.Add(Actor);
			}
		}
		OnHitsChecked(Hits);
	};

	if (AssassinsInterframeHitCheck::Mode == 0)
	{
		SweepMoveAlone(Mover, Shape, CanHit, Collect, Location, Location);
		Finish();
		return;
	}

	// 순간은 이번 프레임 끝의 길이 0인 구간이다. 대상은 그 프레임 기록 위치에 서 있다(SweepAgainstTarget).
	FPendingHitCheck& Pending = PendingHitChecks.AddDefaulted_GetRef();
	Pending.Check.OwnerHandle = ++LastHandle;
	Pending.Check.Mover = Mover;
	Pending.Check.MoverFrom = Location;
	Pending.Check.MoverTo = Location;
	Pending.Check.MoverRadius = Shape.Radius;
	Pending.Check.MoverHalfHeight = Shape.HalfHeight;
	Pending.Check.CanHit = MoveTemp(CanHit);
	Pending.Check.OnHit = MoveTemp(Collect);
	Pending.Span = EPendingSpan::FrameEnd;
	Pending.OnDone = MoveTemp(Finish);
}

// 투사체 한 비행의 판정 모양과 콜백을 등록한다. 비행 동안 바뀌지 않아, 구간마다는 시각과 위치만 넘긴다(CheckProjectileSpanHit).
// 구간 판정은 프레임 끝에 모아 시각 순으로 넘긴다. 그래서 첫 대상에서 멈추는 투사체는 먼저 판정한 대상이 아니라 먼저 닿은
// 대상을 맞힌다.
int32 UAssassinsInterframeHitCheckSubsystem::BeginProjectileHitChecks(AActor* Projectile, const FAssassinsInterframeHitShape& Shape, FCanHit CanHit, FOnHit OnHit)
{
	if ((Projectile == nullptr) || !CanHit || !OnHit || !IsServer())
	{
		return 0;
	}

	FProjectileHitChecks& Registration = ProjectileHitChecks.AddDefaulted_GetRef();
	Registration.Handle = ++LastHandle;
	Registration.Projectile = Projectile;
	Registration.Shape = Shape;
	Registration.CanHit = MoveTemp(CanHit);
	Registration.OnHit = MoveTemp(OnHit);
	return Registration.Handle;
}

void UAssassinsInterframeHitCheckSubsystem::CheckProjectileSpanHit(int32 Handle, double FromTime, const FVector& From, double ToTime, const FVector& To)
{
	SCOPE_CYCLE_COUNTER(STAT_AssassinsInterframeHitCheck);

	const FProjectileHitChecks* Found = ProjectileHitChecks.FindByPredicate([Handle](const FProjectileHitChecks& Each) { return Each.Handle == Handle; });
	if ((Found == nullptr) || !Found->Projectile.IsValid() || !IsServer() || (ToTime <= FromTime))
	{
		return;
	}

	// 명중이 투사체를 끝내 등록을 지울 수 있어 복사해 둔다.
	const FProjectileHitChecks Registration = *Found;
	if (AssassinsInterframeHitCheck::Mode == 0)
	{
		SweepMoveAlone(Registration.Projectile.Get(), Registration.Shape, Registration.CanHit, Registration.OnHit, From, To);
		return;
	}

	// 같은 프레임에 이 뒤에 틱하는 캐릭터의 위치까지 기록한 뒤에 판정한다(RunPendingHitChecks).
	FPendingHitCheck& Pending = PendingHitChecks.AddDefaulted_GetRef();
	Pending.Check.OwnerHandle = Handle;
	Pending.Check.Mover = Registration.Projectile;
	Pending.Check.TargetFromTime = FromTime;
	Pending.Check.TargetToTime = ToTime;
	Pending.Check.MoverFrom = From;
	Pending.Check.MoverTo = To;
	Pending.Check.MoverRadius = Registration.Shape.Radius;
	Pending.Check.MoverHalfHeight = Registration.Shape.HalfHeight;
	Pending.Check.MoverFromTime = FromTime;
	Pending.Check.MoverToTime = ToTime;
	Pending.Check.CanHit = Registration.CanHit;
	Pending.Check.OnHit = Registration.OnHit;
}

bool UAssassinsInterframeHitCheckSubsystem::HasPendingHitChecks(int32 Handle) const
{
	return (Handle != 0) && PendingHitChecks.ContainsByPredicate([Handle](const FPendingHitCheck& Each) { return Each.Check.OwnerHandle == Handle; });
}

void UAssassinsInterframeHitCheckSubsystem::CancelHitChecks(int32 Handle)
{
	PendingHitChecks.RemoveAll([Handle](const FPendingHitCheck& Each) { return Each.Check.OwnerHandle == Handle; });

	// 넘기는 핸들의 명중이 그 소스를 끝냈으면 뒤의 명중도 버린다.
	if (DeliveryDepth > 0)
	{
		CancelledDuringDelivery.Add(Handle);
	}
}

void UAssassinsInterframeHitCheckSubsystem::EndProjectileHitChecks(int32 Handle)
{
	CancelHitChecks(Handle);
	ProjectileHitChecks.RemoveAll([Handle](const FProjectileHitChecks& Each) { return Each.Handle == Handle; });
}

void UAssassinsInterframeHitCheckSubsystem::CheckDashMove(ACharacter* Dasher, const FVector& From, const FVector& To)
{
	// 명중이 어빌리티를 끝내 모양을 뗄 수 있어 복사한다.
	TArray<FDashHitShape, TInlineAllocator<2>> DasherShapes;
	for (const FDashHitShape& HitShape : DashHitShapes)
	{
		if (HitShape.Dasher.Get() == Dasher)
		{
			DasherShapes.Add(HitShape);
		}
	}

	for (const FDashHitShape& HitShape : DasherShapes)
	{
		// 앞서 판정한 모양의 명중 뒤에도 아직 켜져 있는지 본다.
		const int32 Handle = HitShape.Handle;
		if (DashHitShapes.ContainsByPredicate([Handle](const FDashHitShape& Other) { return Other.Handle == Handle; }))
		{
			if (AssassinsInterframeHitCheck::Mode == 0)
			{
				SweepMoveAlone(Dasher, HitShape.Shape, HitShape.CanHit, HitShape.OnHit, From, To);
			}
			else
			{
				DeferDashMove(HitShape, From, To);
			}
		}
	}
}

void UAssassinsInterframeHitCheckSubsystem::DeferDashMove(const FDashHitShape& HitShape, const FVector& From, const FVector& To)
{
	// 이동은 프레임 중에 일어난다. 같은 프레임에 뒤에 틱하는 대상까지 기록한 뒤, 직전 프레임에서 이번 프레임까지의 구간으로 판정한다.
	// 어빌리티가 같은 프레임에 모양을 떼도 이 이동은 판정한다. 한 프레임에 이동이 여럿 오면(원격 플레이어) 기록처럼 프레임 단위로
	// 이어 붙여 하나의 구간으로 판정한다. 끊긴 이동(순간이동)은 따로 판정한다.
	for (FPendingHitCheck& Pending : PendingHitChecks)
	{
		if ((Pending.Span == EPendingSpan::Frame) && (Pending.Check.OwnerHandle == HitShape.Handle) && Pending.Check.MoverTo.Equals(From))
		{
			Pending.Check.MoverTo = To;
			return;
		}
	}

	FPendingHitCheck& Pending = PendingHitChecks.AddDefaulted_GetRef();
	Pending.Check.OwnerHandle = HitShape.Handle;
	Pending.Check.Mover = HitShape.Dasher;
	Pending.Check.MoverFrom = From;
	Pending.Check.MoverTo = To;
	Pending.Check.MoverRadius = HitShape.Shape.Radius;
	Pending.Check.MoverHalfHeight = HitShape.Shape.HalfHeight;
	Pending.Check.CanHit = HitShape.CanHit;
	Pending.Check.OnHit = HitShape.OnHit;
	Pending.Span = EPendingSpan::Frame;
}

bool UAssassinsInterframeHitCheckSubsystem::PassesBroadPhase(const FHitCheck& Check, const FCharacterRecord& TargetRecord) const
{
	// 넓은 단계가 들어갈 자리. 대상의 구간 상자와 주체의 구간 상자가 겹치지 않으면 거짓을 돌려줄 수 있다. 지금은 모두 정밀 판정한다.
	return true;
}

void UAssassinsInterframeHitCheckSubsystem::CheckHitAgainstAllCharacters(const FHitCheck& Check, TArray<FHitCheck, TInlineAllocator<4>>& OutHits) const
{
	const AActor* Mover = Check.Mover.Get();

	for (const TPair<TObjectKey<ACharacter>, FCharacterRecord>& Entry : CharacterRecords)
	{
		ACharacter* Target = Entry.Value.Character.Get();
		if ((Target == nullptr) || (Target == Mover))
		{
			continue;
		}

		FHitCheck TargetCheck = Check;
		TargetCheck.Target = Target;
		if (!PassesBroadPhase(TargetCheck, Entry.Value) || !Check.CanHit(Target))
		{
			continue;
		}

		if (SweepAgainstTarget(TargetCheck, Entry.Value))
		{
			OutHits.Add(MoveTemp(TargetCheck));
		}
	}
}

void UAssassinsInterframeHitCheckSubsystem::SweepMoveAlone(AActor* Mover, const FAssassinsInterframeHitShape& HitShape, const FCanHit& CanHit, const FOnHit& OnHit, const FVector& From, const FVector& To) const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FCollisionShape Shape = FCollisionShape::MakeCapsule(HitShape.Radius, HitShape.HalfHeight);
	const FCollisionObjectQueryParams ObjectQueryParams(ECC_Pawn);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AssassinsInterframeHitCheckSweep), false, Mover);

	// 액터마다 한 번만 찾는다(어느 컴포넌트가 잡히든). 제자리 이동도 그 자리에 선 대상을 찾는다.
	TArray<TWeakObjectPtr<AActor>, TInlineAllocator<8>> Found;
	if (From.Equals(To))
	{
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, To, FQuat::Identity, ObjectQueryParams, Shape, QueryParams);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			Found.AddUnique(Overlap.GetActor());
		}
	}
	else
	{
		TArray<FHitResult> Hits;
		World->SweepMultiByObjectType(Hits, From, To, FQuat::Identity, ObjectQueryParams, Shape, QueryParams);
		for (const FHitResult& Hit : Hits)
		{
			Found.AddUnique(Hit.GetActor());
		}
	}

	for (const TWeakObjectPtr<AActor>& FoundActor : Found)
	{
		AActor* Target = FoundActor.Get();
		if ((Target != nullptr) && CanHit(Target))
		{
			OnHit(Target);
		}
	}

}

bool UAssassinsInterframeHitCheckSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return (World != nullptr) && (World->GetNetMode() != NM_Client);
}

bool UAssassinsInterframeHitCheckSubsystem::SweepAgainstTarget(FHitCheck& Check, const FCharacterRecord& TargetRecord) const
{
	const ACharacter* Target = Check.Target.Get();
	const UCapsuleComponent* TargetCapsule = Target ? Target->GetCapsuleComponent() : nullptr;
	if (TargetCapsule == nullptr)
	{
		return false;
	}

	// 판정 구간은 대상의 직전 기록과 현재 기록 사이다. 돌진과 순간 판정은 그 양 끝이고, 투사체는 사거리에서 잘려 사이에서 끝날 수 있다.
	// 순간이동으로 온 기록이면 그 사이를 지나간 것이 아니라 구간은 판정하지 않는다. 구간 길이가 0이면(순간 판정) 대상은 그 시점
	// 위치에 서 있어 판정한다.
	const double From = Check.TargetFromTime;
	const double To = Check.TargetToTime;
	if ((To > From) && TargetRecord.Current.bTeleported)
	{
		return false;
	}

	FAssassinsSweptPair Pair;
	Pair.StartA = Check.MoverFrom;
	Pair.EndA = Check.MoverTo;
	Pair.RadiusA = Check.MoverRadius;
	Pair.HalfHeightA = Check.MoverHalfHeight;
	Pair.StartB = TargetRecord.LocationAt(From);
	Pair.EndB = TargetRecord.LocationAt(To);
	Pair.RadiusB = TargetCapsule->GetScaledCapsuleRadius();
	Pair.HalfHeightB = TargetCapsule->GetScaledCapsuleHalfHeight();
	const FAssassinsSweptPairResult Result = AssassinsInterframeHitCheck::SolvePair(Pair);

	if (Result.bHit)
	{
		Check.TargetHitTime = From + Result.TimeOfImpact * (To - From);
		Check.MoverHitTime = FMath::Lerp(Check.MoverFromTime, Check.MoverToTime, Result.TimeOfImpact);
	}

	return Result.bHit;
}

void UAssassinsInterframeHitCheckSubsystem::DeliverHits(TArray<FHitCheck, TInlineAllocator<4>>& Hits)
{
	if (Hits.Num() == 0)
	{
		return;
	}
	for (const FHitCheck& Hit : Hits)
	{
		LogHit(Hit);
	}
	Hits.Sort([](const FHitCheck& A, const FHitCheck& B) { return A.MoverHitTime < B.MoverHitTime; });

	// 한 핸들의 여러 이동 판정이 한 캐릭터에 함께 명중할 수 있어 가장 이른 것만 쓴다.
	TArray<FHitCheck, TInlineAllocator<8>> FirstHits;
	for (FHitCheck& Hit : Hits)
	{
		const bool bAlreadyHit = FirstHits.ContainsByPredicate([&Hit](const FHitCheck& First)
		{
			return (First.OwnerHandle == Hit.OwnerHandle) && (First.Target == Hit.Target);
		});
		if (!bAlreadyHit)
		{
			FirstHits.Add(MoveTemp(Hit));
		}
	}

	// 기록을 다 훑은 뒤에 넘긴다. 명중이 대상의 플레이를 끝내 없애거나, 소스를 끝내 뒤 명중을 버릴 수 있다(CancelHitChecks).
	++DeliveryDepth;
	for (FHitCheck& Hit : FirstHits)
	{
		ACharacter* Target = Hit.Target.Get();
		if ((Target != nullptr) && !CancelledDuringDelivery.Contains(Hit.OwnerHandle))
		{
			Hit.OnHit(Target);
		}
	}
	if (--DeliveryDepth == 0)
	{
		CancelledDuringDelivery.Reset();
	}
}

void UAssassinsInterframeHitCheckSubsystem::LogHit(const FHitCheck& Check)
{
	UE_LOG(LogAssassinsAbilitySystem, Log, TEXT("InterframeHitCheck: %s hits %s at %.4f on the target's record(over %.4f to %.4f), as the frame ends."),
		*GetNameSafe(Check.Mover.Get()), *GetNameSafe(Check.Target.Get()), Check.TargetHitTime, Check.TargetFromTime, Check.TargetToTime);
}

// 모든 액터의 틱이 끝난 뒤 서버는 프레임을 기록하고 쌓인 판정을 한다.
void UAssassinsInterframeHitCheckSubsystem::HandlePostActorTick(UWorld* TickedWorld, ELevelTick TickType, float DeltaSeconds)
{
	if ((TickedWorld != GetWorld()) || !IsServer())
	{
		return;
	}

	RecordPositions();
	RunPendingHitChecks();
}
