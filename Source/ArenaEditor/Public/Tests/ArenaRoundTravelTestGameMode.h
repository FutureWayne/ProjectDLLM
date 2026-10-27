// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameMode/ArenaGameMode.h"
#include "ArenaRoundTravelTestGameMode.generated.h"

/** Uses real player migration without spawning gameplay pawns. */
UCLASS(Transient, NotBlueprintable)
class AArenaRoundTravelTestGameMode : public AArenaGameMode
{
	GENERATED_BODY()

public:
	AArenaRoundTravelTestGameMode();
	virtual void RestartPlayer(AController* NewPlayer) override;
};
