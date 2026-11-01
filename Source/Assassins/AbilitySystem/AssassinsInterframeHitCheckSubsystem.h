#pragma once

#include "Engine/EngineBaseTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"

#include "AssassinsInterframeHitCheckSubsystem.generated.h"

class ACharacter;

struct FAssassinsSweptPair
{
	FVector StartA = FVector::ZeroVector;
	FVector EndA = FVector::ZeroVector;
	float RadiusA = 0.0f;
	float HalfHeightA = 0.0f;

	FVector StartB = FVector::ZeroVector;
	FVector EndB = FVector::ZeroVector;
	float RadiusB = 0.0f;
	float HalfHeightB = 0.0f;
};

struct FAssassinsSweptPairResult
{
	bool bHit = false;

	double TimeOfImpact = 1.0;

	double ClosestTime = 0.0;
	double ClosestDistance = 0.0;
};

struct FAssassinsInterframeHitShape
{
	float Radius = 0.0f;
	float HalfHeight = 0.0f;
};

namespace AssassinsInterframeHitCheck
{
	ASSASSINS_API FAssassinsSweptPairResult SolvePair(const FAssassinsSweptPair& Pair);
}

UCLASS()
class ASSASSINS_API UAssassinsInterframeHitCheckSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	using FCanHit = TFunction<bool(AActor*)>;
	using FOnHit = TFunction<void(AActor*)>;
	using FOnHitsChecked = TFunction<void(const TArray<AActor*>&)>;

	static UAssassinsInterframeHitCheckSubsystem* Get(const UObject* WorldContextObject);

	static bool UsesServerRecords();

	void Register(ACharacter* Character);
	void Unregister(ACharacter* Character);

	void OnServerCharacterMoved(ACharacter* Character, const FVector& OldLocation);

	void RecordTeleport(ACharacter* Character);

	void RecordPositions();
	void RunPendingHitChecks();

	int32 BeginDashHitShape(ACharacter* Dasher, const FAssassinsInterframeHitShape& Shape, FCanHit CanHit, FOnHit OnHit);
	void EndDashHitShape(int32 Handle);

	void CheckHitAt(ACharacter* Mover, const FVector& Location, const FAssassinsInterframeHitShape& Shape, FCanHit CanHit, FOnHitsChecked OnHitsChecked);

	int32 BeginProjectileHitChecks(AActor* Projectile, const FAssassinsInterframeHitShape& Shape, FCanHit CanHit, FOnHit OnHit);
	void CheckProjectileSpanHit(int32 Handle, double FromTime, const FVector& From, double ToTime, const FVector& To);

	bool HasPendingHitChecks(int32 Handle) const;

	void CancelHitChecks(int32 Handle);

	void EndProjectileHitChecks(int32 Handle);

protected:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	struct FPositionRecord
	{
		double Time = 0.0;
		FVector Location = FVector::ZeroVector;

		bool bTeleported = false;
	};

	struct FCharacterRecord
	{
		TWeakObjectPtr<ACharacter> Character;

		FPositionRecord Previous;
		FPositionRecord Current;

		bool bTeleportRecorded = false;

		FVector LocationAt(double Time) const;
	};

	struct FHitCheck
	{
		int32 OwnerHandle = 0;

		TWeakObjectPtr<AActor> Mover;
		TWeakObjectPtr<ACharacter> Target;

		double TargetFromTime = 0.0;
		double TargetToTime = 0.0;
		FVector MoverFrom = FVector::ZeroVector;
		FVector MoverTo = FVector::ZeroVector;
		float MoverRadius = 0.0f;
		float MoverHalfHeight = 0.0f;

		double MoverFromTime = 0.0;
		double MoverToTime = 1.0;

		double TargetHitTime = 0.0;
		double MoverHitTime = 0.0;

		FCanHit CanHit;
		FOnHit OnHit;
	};

	enum class EPendingSpan : uint8
	{
		Given,
		Frame,
		FrameEnd
	};

	struct FPendingHitCheck
	{
		FHitCheck Check;
		EPendingSpan Span = EPendingSpan::Given;
		TFunction<void()> OnDone;
	};

	struct FDashHitShape
	{
		int32 Handle = 0;
		TWeakObjectPtr<ACharacter> Dasher;
		FAssassinsInterframeHitShape Shape;
		FCanHit CanHit;
		FOnHit OnHit;
	};

	struct FProjectileHitChecks
	{
		int32 Handle = 0;
		TWeakObjectPtr<AActor> Projectile;
		FAssassinsInterframeHitShape Shape;
		FCanHit CanHit;
		FOnHit OnHit;
	};

	bool IsServer() const;

	void CheckDashMove(ACharacter* Dasher, const FVector& From, const FVector& To);
	void DeferDashMove(const FDashHitShape& HitShape, const FVector& From, const FVector& To);

	void CheckHitAgainstAllCharacters(const FHitCheck& Check, TArray<FHitCheck, TInlineAllocator<4>>& OutHits) const;
	bool PassesBroadPhase(const FHitCheck& Check, const FCharacterRecord& TargetRecord) const;

	void SweepMoveAlone(AActor* Mover, const FAssassinsInterframeHitShape& Shape, const FCanHit& CanHit, const FOnHit& OnHit, const FVector& From, const FVector& To) const;

	bool SweepAgainstTarget(FHitCheck& Check, const FCharacterRecord& TargetRecord) const;

	void HandlePostActorTick(UWorld* TickedWorld, ELevelTick TickType, float DeltaSeconds);

	void DeliverHits(TArray<FHitCheck, TInlineAllocator<4>>& Hits);

	static void LogHit(const FHitCheck& Check);

	TMap<TObjectKey<ACharacter>, FCharacterRecord> CharacterRecords;
	TArray<FPendingHitCheck> PendingHitChecks;
	double PreviousRecordedTime = -1.0;
	double LatestRecordedTime = -1.0;

	TArray<FDashHitShape> DashHitShapes;
	TArray<FProjectileHitChecks> ProjectileHitChecks;

	int32 LastHandle = 0;

	TSet<int32> CancelledDuringDelivery;
	int32 DeliveryDepth = 0;

	FDelegateHandle PostActorTickHandle;
};
