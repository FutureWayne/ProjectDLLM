// Copyright Ludens Studio. All Rights Reserved.


#include "Player/ArenaLocalPlayer.h"

UArenaLocalPlayer::UArenaLocalPlayer()
{
}

void UArenaLocalPlayer::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	// Do nothing, we merely observe the team of our associated player controller
}

FGenericTeamId UArenaLocalPlayer::GetGenericTeamId() const
{
	if (IArenaTeamAgentInterface* ControllerAsTeamProvider = Cast<IArenaTeamAgentInterface>(PlayerController))
	{
		return ControllerAsTeamProvider->GetGenericTeamId();
	}
	else
	{
		return FGenericTeamId::NoTeam;
	}
}

FOnArenaTeamIndexChangedDelegate* UArenaLocalPlayer::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}
