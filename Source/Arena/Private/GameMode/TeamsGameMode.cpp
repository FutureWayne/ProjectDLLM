// Copyright Ludens Studio. All Rights Reserved.


#include "GameMode/TeamsGameMode.h"

#include "GameState/ArenaGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ArenaPlayerState.h"

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Sort the players into teams
	// If there is player in attack team, assign new player to defense team
	if (AArenaGameState* ArenaGS = Cast<AArenaGameState>(UGameplayStatics::GetGameState(this)))
	{
		if (AArenaPlayerState* ArenaPS = NewPlayer->GetPlayerState<AArenaPlayerState>())
		{
			if (ArenaGS->AttackTeam.Num() == 0)
			{
				ArenaPS->SetTeam(ETeam::ET_Attack);
				ArenaGS->AttackTeam.AddUnique(ArenaPS);
			}
			else
			{
				ArenaPS->SetTeam(ETeam::ET_Defense);
				ArenaGS->DefenseTeam.AddUnique(ArenaPS);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Remove exiting player from the team
	if (AArenaGameState* ArenaGS = Cast<AArenaGameState>(UGameplayStatics::GetGameState(this)))
	{
		if (AArenaPlayerState* ArenaPS = Exiting->GetPlayerState<AArenaPlayerState>())
		{
			if (ArenaGS->AttackTeam.Contains(ArenaPS))
			{
				ArenaGS->AttackTeam.Remove(ArenaPS);
			}
			else if (ArenaGS->DefenseTeam.Contains(ArenaPS))
			{
				ArenaGS->DefenseTeam.Remove(ArenaPS);
			}
		}
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// Sort the players into teams
	if (AArenaGameState* ArenaGS = Cast<AArenaGameState>(UGameplayStatics::GetGameState(this)))
	{
		for (auto PlayerState : ArenaGS->PlayerArray)
		{
			if (AArenaPlayerState* ArenaPS = Cast<AArenaPlayerState>(PlayerState); ArenaPS && ArenaPS->GetTeam() == ETeam::ET_Neutral)
			{
				// Assign one player to attack and the rest to defense
				if (ArenaGS->AttackTeam.Num() == 0)
				{
					ArenaPS->SetTeam(ETeam::ET_Attack);
					ArenaGS->AttackTeam.AddUnique(ArenaPS);
				}
				else
				{
					ArenaPS->SetTeam(ETeam::ET_Defense);
					ArenaGS->DefenseTeam.AddUnique(ArenaPS);
				}
			}
		}
	}
}
