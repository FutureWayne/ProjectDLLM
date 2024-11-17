// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ArenaGameState.generated.h"

class AArenaPlayerState;
/**
 * 
 */
UCLASS()
class ARENA_API AArenaGameState : public AGameState
{
	GENERATED_BODY()

public:
	TArray<AArenaPlayerState*> AttackTeam;
	TArray<AArenaPlayerState*> DefenseTeam;

	UFUNCTION(BlueprintCallable)
	AActor* GetAttackTeamTarget();

	UFUNCTION(BlueprintCallable)
	void SetAttackTeamTarget(AActor* Target);
private:
	AActor* AttackTeamTarget;
};
