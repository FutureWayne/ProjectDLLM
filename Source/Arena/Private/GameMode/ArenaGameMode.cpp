// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/ArenaGameMode.h"

#include "GameState/ArenaGameState.h"
#include "Player/ArenaPlayerState.h"

AArenaGameMode::AArenaGameMode(const FObjectInitializer& ObjectInitializer)
{
	GameStateClass = AArenaGameState::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	PlayerStateClass = AArenaPlayerState::StaticClass();
}

void AArenaGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);

	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void AArenaGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
}
