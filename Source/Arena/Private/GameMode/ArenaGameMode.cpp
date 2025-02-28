// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/ArenaGameMode.h"

#include "GameState/ArenaGameState.h"
#include "Player/ArenaPlayerSpawningManagerComponent.h"
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

AActor* AArenaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (UArenaPlayerSpawningManagerComponent* SpawningManager = GameState->FindComponentByClass<UArenaPlayerSpawningManagerComponent>())
	{
		return SpawningManager->ChoosePlayerStart(Player);
	}
	
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AArenaGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (UArenaPlayerSpawningManagerComponent* SpawningManager = GameState->FindComponentByClass<UArenaPlayerSpawningManagerComponent>())
	{
		SpawningManager->FinishRestartPlayer(NewPlayer, StartRotation);
	}
	
	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool AArenaGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	// Do nothing, we'll wait until PostLogin when we try to spawn the player for real.
	// Doing anything right now is no good, systems like team assignment haven't even occurred yet.
	return true;
}

void AArenaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

bool AArenaGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// We never want to use the start spot, always use the spawn management component.
	return false;
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
