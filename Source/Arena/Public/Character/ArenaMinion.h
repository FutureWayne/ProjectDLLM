// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Character/ArenaCharacterBase.h"
#include "ArenaMinion.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AArenaMinion : public AArenaCharacterBase, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AArenaMinion();

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
