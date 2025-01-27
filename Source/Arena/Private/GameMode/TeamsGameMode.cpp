// Copyright Ludens Studio. All Rights Reserved.


#include "GameMode/TeamsGameMode.h"

#include "Character/BlasterCharacter.h"
#include "GameState/ArenaGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ArenaPlayerState.h"
#include "PlayerController/ArenaPlayerController.h"
#include "Teams/ArenaTeamSubsystem.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

ATeamsGameMode::ATeamsGameMode()
{
	bDelayedStart = false;
}

void ATeamsGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = AgentChoosingDuration - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.0f)
		{
			StartMatch();
		}
		// }
		// else if (MatchState == MatchState::InProgress)
		// {
		// 	CountdownTime = AgentChoosingDuration + MatchDuration - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		// 	if (CountdownTime <= 0.0f)
		// 	{
		// 		SetMatchState(MatchState::Cooldown);
		// 	}
		// }
		// else if (MatchState == MatchState::Cooldown)
		// {
		// 	CountdownTime = AgentChoosingDuration + MatchDuration + CooldownDuration - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		// 	if (CountdownTime <= 0.0f)
		// 	{
		// 		RestartGame();
		// 	}
		// }
	}
}

void ATeamsGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Error, TEXT("PostLogin"));
	
	if (AArenaGameState* ArenaGS = Cast<AArenaGameState>(UGameplayStatics::GetGameState(this)))
	{
		if (AArenaPlayerState* ArenaPS = NewPlayer->GetPlayerState<AArenaPlayerState>())
		{
			UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
			if (ensure(TeamSubsystem))
			{
				int32 TotalPlayers = ArenaGS->AttackTeam.Num() + ArenaGS->DefenseTeam.Num();
				if (ArenaGS->AttackTeam.Num() <= TotalPlayers / 2)
				{
					TeamSubsystem->ChangeTeamForActor(ArenaPS, 1);
					ArenaGS->AttackTeam.AddUnique(ArenaPS);
				}
				else
				{
					TeamSubsystem->ChangeTeamForActor(ArenaPS, 2);
					ArenaGS->DefenseTeam.AddUnique(ArenaPS);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("TeamSubsystem is not valid"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ArenaPlayerState is not valid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ArenaGameState is not valid"));
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

	UE_LOG(LogTemp, Error, TEXT("HandleMatchHasStarted"));

	// Sort the players into teams
	if (AArenaGameState* ArenaGS = Cast<AArenaGameState>(UGameplayStatics::GetGameState(this)))
	{
		UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			for (auto PlayerState : ArenaGS->PlayerArray)
			{
				if (AArenaPlayerState* ArenaPS = Cast<AArenaPlayerState>(PlayerState); ArenaPS && ArenaPS->GetTeamId() == INDEX_NONE)
				{
					if (ArenaGS->AttackTeam.Num() <= ArenaGS->DefenseTeam.Num())
					{
						TeamSubsystem->ChangeTeamForActor(PlayerState, 1);
						ArenaGS->AttackTeam.AddUnique(ArenaPS);
					}
					else
					{
						TeamSubsystem->ChangeTeamForActor(PlayerState, 2);
						ArenaGS->DefenseTeam.AddUnique(ArenaPS);
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("PlayerState is not valid"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("TeamSubsystem is not valid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ArenaGameState is not valid"));
	}
}

void ATeamsGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AArenaPlayerController* ArenaPC = Cast<AArenaPlayerController>(*It))
		{
			ArenaPC->OnMatchStateSet(MatchState);
		}
	}
}

UClass* ATeamsGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	bool bIsPartOfTeam = false;
	int32 TeamId = INDEX_NONE;
	FLinearColor TeamColor = FLinearColor::White;

	int32 Minutes = FMath::FloorToInt(60 / 60.f);
	int32 Seconds = 60 % 60;

	UArenaTeamSubsystem::FindTeamFromObject(InController, bIsPartOfTeam, TeamId, TeamColor);
	if (TeamId == 1)
	{
		return AttackerCharacterClass;
	}
	else if (TeamId == 2)
	{
		return DefenderCharacterClass;
	}
	else
	{
		return Super::GetDefaultPawnClassForController_Implementation(InController);
	}
}
