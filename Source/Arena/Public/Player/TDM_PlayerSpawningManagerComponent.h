// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/ArenaPlayerSpawningManagerComponent.h"
#include "TDM_PlayerSpawningManagerComponent.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UTDM_PlayerSpawningManagerComponent : public UArenaPlayerSpawningManagerComponent
{
	GENERATED_BODY()

public:
	UTDM_PlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer);

	virtual AActor* OnChoosePlayerStart(AController* Player, TArray<AArenaPlayerStart*>& PlayerStarts) override;
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) override;
};
