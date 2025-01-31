// Copyright Ludens Studio. All Rights Reserved.


#include "GameMode/TeamsGameMode.h"

#include "Character/ArenaCharacter.h"
#include "GameState/ArenaGameState.h"
#include "Teams/ArenaTeamSubsystem.h"

ATeamsGameMode::ATeamsGameMode()
{
	bDelayedStart = false;
}

UClass* ATeamsGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	bool bIsPartOfTeam = false;
	int32 TeamId = INDEX_NONE;
	FLinearColor TeamColor = FLinearColor::White;

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
