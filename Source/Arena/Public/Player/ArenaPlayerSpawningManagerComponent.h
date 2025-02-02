// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "ArenaPlayerSpawningManagerComponent.generated.h"

class AArenaPlayerStart;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaPlayerSpawningManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UArenaPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer);

	// ~Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// ~End UActorComponent Interface

protected:
	// Utility
	APlayerStart* GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<AArenaPlayerStart*>& FoundStartPoints) const;

	virtual AActor* OnChoosePlayerStart(AController* Player, TArray<AArenaPlayerStart*>& PlayerStarts) { return nullptr; }
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) { }

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnFinishRestartPlayer"))
	void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);

private:
	AActor* ChoosePlayerStart(AController* Player);
	bool ControllerCanRestart(AController* Player);
	void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation);
	friend class AArenaGameMode;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AArenaPlayerStart>> CachedPlayerStarts;

private:
	void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);
	void HandleOnActorSpawned(AActor* SpawnedActor);

#if WITH_EDITOR
	APlayerStart* FindPlayFromHereStart(AController* Player);
#endif
};
