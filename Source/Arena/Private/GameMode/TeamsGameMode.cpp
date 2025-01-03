// Copyright Ludens Studio. All Rights Reserved.


#include "GameMode/TeamsGameMode.h"

#include "Character/BlasterCharacter.h"
#include "GameState/ArenaGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ArenaPlayerState.h"
#include "PlayerController/ArenaPlayerController.h"

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
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = AgentChoosingDuration + MatchDuration - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.0f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = AgentChoosingDuration + MatchDuration + CooldownDuration - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.0f)
		{
			RestartGame();
		}
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

	// Sort the players into teams
	// If there is player in attack team, assign new player to defense team
	if (AArenaGameState* ArenaGS = Cast<AArenaGameState>(UGameplayStatics::GetGameState(this)))
	{
		if (AArenaPlayerState* ArenaPS = NewPlayer->GetPlayerState<AArenaPlayerState>())
		{
			UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
			if (ensure(TeamSubsystem))
			{
				if (ArenaGS->AttackTeam.Num() == 0)
				{
					TeamSubsystem->ChangeTeamForActor(ArenaPS, ETeam::ET_Attack);
					ArenaGS->AttackTeam.AddUnique(ArenaPS);
				}
				else
				{
					TeamSubsystem->ChangeTeamForActor(ArenaPS, ETeam::ET_Defense);
					ArenaGS->DefenseTeam.AddUnique(ArenaPS);
				}
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
			if (AArenaPlayerState* ArenaPS = Cast<AArenaPlayerState>(PlayerState); ArenaPS && ArenaPS->GetTeam() == ETeam::ET_Max)
			{
				UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
				if (ensure(TeamSubsystem))
				{
					// Assign one player to attack and the rest to defense
					if (ArenaGS->AttackTeam.Num() == 0)
					{
						TeamSubsystem->ChangeTeamForActor(ArenaPS, ETeam::ET_Attack);
						ArenaGS->AttackTeam.AddUnique(ArenaPS);
					}
					else
					{
						TeamSubsystem->ChangeTeamForActor(ArenaPS, ETeam::ET_Defense);
						ArenaGS->DefenseTeam.AddUnique(ArenaPS);
					}
				}
					
				
			}
		}
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
	ETeam TeamId = ETeam::ET_Max;
	FLinearColor TeamColor = FLinearColor::White;

	UArenaTeamSubsystem::FindTeamFromObject(InController, bIsPartOfTeam, TeamId, TeamColor);
	if (TeamId == ETeam::ET_Attack)
	{
		return AttackerCharacterClass;
	}
	else if (TeamId == ETeam::ET_Defense)
	{
		return DefenderCharacterClass;
	}
	else
	{
		return Super::GetDefaultPawnClassForController_Implementation(InController);
	}
}
