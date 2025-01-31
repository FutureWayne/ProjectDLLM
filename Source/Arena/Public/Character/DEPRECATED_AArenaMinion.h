// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ArenaCharacter.h"
#include "Teams/ArenaTeamAgentInterface.h"
#include "DEPRECATED_AArenaMinion.generated.h"

/**
 * 
 */
UCLASS(Deprecated)
class ARENA_API ADEPRECATED_AArenaMinion : public AArenaCharacter, public IArenaTeamAgentInterface
{
	GENERATED_BODY()

public:
	ADEPRECATED_AArenaMinion();

	// ~ Begin IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override { return MyTeamID; }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	// ~ End IGenericTeamAgentInterface

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Replicated)
	FGenericTeamId MyTeamID;
};
