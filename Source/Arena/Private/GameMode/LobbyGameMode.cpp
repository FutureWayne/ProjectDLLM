// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Print current player count
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Current Player Count: %d"), GameState.Get()->PlayerArray.Num()));

	if (GameState.Get()->PlayerArray.Num() >= 4)
	{
		StartGame();
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void ALobbyGameMode::StartGame()
{
	if (UWorld* World = GetWorld())
	{
		bUseSeamlessTravel = true;
		World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
	}
}
