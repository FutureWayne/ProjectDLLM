// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ArenaGameMode.generated.h"


/**
 * Post login event, triggered when a player or bot joins the game as well as after seamless and non seamless travel
 *
 * This is called after the player has finished initialization
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnArenaGameModePlayerInitialized, AGameModeBase* /*GameMode*/, AController* /*NewPlayer*/);


/**
 * 
 */
UCLASS()
class ARENA_API AArenaGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AArenaGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AGameModeBase interface
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;
	//~End of AGameModeBase interface

	UFUNCTION(BlueprintCallable)
	void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);

	// Delegate called on player initialization, described above 
	FOnArenaGameModePlayerInitialized OnGameModePlayerInitialized;
};
