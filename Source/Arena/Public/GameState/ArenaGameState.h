// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/GameState.h"
#include "GameplayCueInterface.h"
#include "GameState/ArenaMatchFinaleTypes.h"
#include "ArenaGameState.generated.h"

struct FArenaVerbMessage;
class UArenaAbilitySystemComponent;
class AArenaPlayerState;

DECLARE_MULTICAST_DELEGATE_TwoParams(FArenaMatchFinaleStageChangedNative, EArenaMatchFinaleStage /*NewStage*/, EArenaMatchFinaleStage /*OldStage*/);

/**
 * 
 */
UCLASS()
class ARENA_API AArenaGameState : public AGameState, public IAbilitySystemInterface, public IGameplayCueInterface
{
	GENERATED_BODY()
	
public:
	AArenaGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of UObject interface

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	//~IGameplayCueInterface
	using IGameplayCueInterface::HandleGameplayCue;
	virtual void HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;
	//~End of IGameplayCueInterface
	
	UFUNCTION(BlueprintCallable, Category = "Arena|GameState")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	const FArenaMatchFinaleData& GetMatchFinaleData() const { return MatchFinaleData; }

	/** Authority only. Clearing bActive also ends the local stage, on the server and on every client. */
	void SetMatchFinaleData(const FArenaMatchFinaleData& NewData);

	/** Finale stage on this machine, set by the stage GameplayCues published by the post-match phase. */
	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	EArenaMatchFinaleStage GetMatchFinaleStage() const { return LocalFinaleStage; }

	/** Real seconds since the current stage arrived on this machine; 0 when no finale is running. */
	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	float GetMatchFinaleStageElapsedSeconds() const;

	/** Real seconds left in the current stage; 0 when no finale is running. */
	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	float GetMatchFinaleStageRemainingSeconds() const;

	/** Elapsed fraction of the current stage in [0, 1]; 1 for zero-length stages or when no finale is running. */
	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	float GetMatchFinaleStageProgress() const;

	/** Raised on the server and on every client whenever the local finale stage changes. */
	FArenaMatchFinaleStageChangedNative OnMatchFinaleStageChanged;

	// Send a message that all clients will (probably) get
	// (use only for client notifications like eliminations, server join messages, etc... that can handle being lost)
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "Arena|GameState")
	void MulticastMessageToClients(const FArenaVerbMessage Message);

	// Send a message that all clients will be guaranteed to get
	// (use only for client notifications that cannot handle being lost)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Arena|GameState")
	void MulticastReliableMessageToClients(const FArenaVerbMessage Message);

protected:
	virtual void OnRep_ReplicatedWorldTimeSecondsDouble() override;

private:
	friend class FArenaMatchFinaleInitialSyncTest;

	UFUNCTION()
	void OnRep_MatchFinaleData(const FArenaMatchFinaleData& OldData);

	void EnterLocalFinaleStage(EArenaMatchFinaleStage NewStage, float StageSeconds);
	void TrySyncInitialFinaleTime();
	void ClearLocalFinaleStage();

	UPROPERTY(VisibleAnywhere, Category = "Arena|GameState")
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(ReplicatedUsing = OnRep_MatchFinaleData)
	FArenaMatchFinaleData MatchFinaleData;

	// Timed locally in real time from when the cue arrives: a client's estimate of server time is a running average
	// that lags several seconds behind time dilation changes such as the finale slow motion.
	EArenaMatchFinaleStage LocalFinaleStage = EArenaMatchFinaleStage::None;
	float LocalFinaleStageSeconds = 0.0f;
	double LocalFinaleStageStartRealTime = 0.0;
	bool bPendingInitialFinaleTimeSync = false;
};
