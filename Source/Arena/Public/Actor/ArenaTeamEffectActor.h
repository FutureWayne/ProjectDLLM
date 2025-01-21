// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actor/ArenaEffectActor.h"
#include "Teams/ArenaTeamAgentInterface.h"
#include "ArenaTeamEffectActor.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AArenaTeamEffectActor : public AArenaEffectActor, public IArenaTeamAgentInterface
{
	GENERATED_BODY()

public:
	// ~ Begin IArenaTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override { return MyTeamID; }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	// ~ End IArenaTeamAgentInterface

	
private:
	UPROPERTY(Replicated)
	FGenericTeamId MyTeamID;
	
};
