// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Teams/AssassinsTeamAgentInterface.h"
#include "ModularCharacter.h"

#include "AssassinsCharacter.generated.h"

class UAssassinsPawnExtensionComponent;
class UAssassinsHealthComponent;
class UAssassinsAbilitySystemComponent;
class UAssassinsCameraComponent;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStatusChangeDelegate);

UCLASS(Blueprintable)
class AAssassinsCharacter : public AModularCharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface, public IAssassinsTeamAgentInterface
{
	GENERATED_BODY()

public:
	AAssassinsCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Assassins|Character")
	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponent() const;
	//~IAbilitySystemInterface interface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface interface

	//~IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	//~End of IGameplayTagAssetInterface

	UFUNCTION(BlueprintCallable, Category = "Assassins|Character|Status")
	void SetGameplayTag(FGameplayTag Tag);
	UFUNCTION(BlueprintCallable, Category = "Assassins|Character|Status")
	void ClearGameplayTag(FGameplayTag Tag);
	UFUNCTION(BlueprintCallable, Category = "Assassins|Character|Status")
	void AddGameplayTag(FGameplayTag Tag);
	UFUNCTION(BlueprintCallable, Category = "Assassins|Character|Status")
	void RemoveGameplayTag(FGameplayTag Tag);
	UFUNCTION(BlueprintPure, Category = "Assassins|Character|Status")
	bool HasGameplayTag(FGameplayTag Tag);

	//~IAssassinsTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~End of IAssassinsTeamAgentInterface interface

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UAssassinsCameraComponent* GetAssassinsCameraComponent() const { return CameraComponent; }

public:

	////////////////////////////////////////////////////////////////////////////////////
	// Character plays montage or the effect causer implements gameplay logic and vfx.
	////////////////////////////////////////////////////////////////////////////////////
	
	UPROPERTY(BlueprintAssignable, Category = "Assassins|Character|Status")
	FStatusChangeDelegate OnChannelingStarted;
	UPROPERTY(BlueprintAssignable, Category = "Assassins|Character|Status")
	FStatusChangeDelegate OnChannelingEnded;

	UPROPERTY(BlueprintAssignable, Category = "Assassins|Character|Status")
	FStatusChangeDelegate OnInvisibilityStarted;
	UPROPERTY(BlueprintAssignable, Category = "Assassins|Character|Status")
	FStatusChangeDelegate OnInvisibilityEnded;

protected:

	//~AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface
	
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void NotifyControllerChanged() override;
	virtual void NotifyRestarted() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	// Bot(AssassinsCharacterWithAbilities) can set its team ID
	void SetTeamId(const FGenericTeamId& NewTeamID);

	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	void InitializeGameplayTags();

	virtual void HandleMoveSpeedChanged(float OldValue, float NewValue);

	////////////////////////////////////////////////////
	// Functions bound to the status tag count changes
	////////////////////////////////////////////////////

	UFUNCTION(BlueprintImplementableEvent, Category = "Assassins|Character|Status", DisplayName = "Handle Generic Gameplay Tag Event")
	void HandleGenericGameplayTagEvent(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnChannelingTagChanged(const FGameplayTag Tag, int32 NewCount);
	UFUNCTION()
	void OnUntargetableTagChanged(const FGameplayTag Tag, int32 NewCount);
	UFUNCTION()
	void OnInvisibleTagChanged(const FGameplayTag Tag, int32 NewCount);
	UFUNCTION()
	void OnDashingTagChanged(const FGameplayTag Tag, int32 NewCount);
	UFUNCTION()
	void OnRootedTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	virtual void HandleDeathStarted();
	UFUNCTION()
	virtual void HandleDeathFinished();

	void DestroyDueToDeath();

	void ResolvePenetrationAfterDash();

	UFUNCTION()
	void OnEndPawnOverlapAfterDash(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Assassins|Character")
	FGameplayTagContainer CharacterOwnedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Character")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

private:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsHealthComponent> HealthComponent;
	
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UAssassinsCameraComponent* CameraComponent;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY()
	TSet<AActor*> ActorsOverlappedAfterDash;

private:

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);
};

