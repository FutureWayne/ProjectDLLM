// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ArenaCharacterBase.h"
#include "ArenaCharacter.generated.h"

class AArenaPlayerState;
/**
 * 
 */
UCLASS()
class ARENA_API AArenaCharacter : public AArenaCharacterBase
{
	GENERATED_BODY()

public:
	AArenaCharacter();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

protected:
	TObjectPtr<AArenaPlayerState> ArenaPlayerState;

	virtual void OnPlayerStateInitialized();

private:
	void InitAbilityActorInfo();
};
