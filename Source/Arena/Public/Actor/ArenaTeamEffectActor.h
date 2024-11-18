// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Actor/ArenaEffectActor.h"
#include "ArenaTeamEffectActor.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AArenaTeamEffectActor : public AArenaEffectActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// ~ Begin IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override { return MyTeamID; }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	// ~ End IGenericTeamAgentInterface

	
private:
	UPROPERTY(Replicated)
	FGenericTeamId MyTeamID;
	
};
