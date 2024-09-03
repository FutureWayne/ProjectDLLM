// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ArenaCharacterBase.h"
#include "ArenaMinion.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AArenaMinion : public AArenaCharacterBase
{
	GENERATED_BODY()

public:
	AArenaMinion();

protected:
	virtual void BeginPlay() override;
};
