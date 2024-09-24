// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ArenaGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AArenaGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AArenaGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);
};
