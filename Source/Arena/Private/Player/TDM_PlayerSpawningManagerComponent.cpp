// Copyright Ludens Studio. All Rights Reserved.


#include "Player/TDM_PlayerSpawningManagerComponent.h"

#include "Player/ArenaPlayerStart.h"
#include "Teams/ArenaTeamSubsystem.h"

UTDM_PlayerSpawningManagerComponent::UTDM_PlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AActor* UTDM_PlayerSpawningManagerComponent::OnChoosePlayerStart(AController* Player,
	TArray<AArenaPlayerStart*>& PlayerStarts)
{
	UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
	if (!ensure(TeamSubsystem))
	{
		return nullptr;
	}

	const int32 PlayerTeamId = TeamSubsystem->FindTeamFromObject(Player);

	if (!ensure(PlayerTeamId != INDEX_NONE))
	{
		return nullptr;
	}

	TArray<AArenaPlayerStart*> TeamStarts;
	AArenaPlayerStart* BestPlayerStart = nullptr;
	AArenaPlayerStart* FallbackPlayerStart = nullptr;
 
	for (AArenaPlayerStart* PlayerStart : PlayerStarts)
	{
		if (PlayerStart->GetTeamId() == PlayerTeamId)
		{
			TeamStarts.Add(PlayerStart);
		}
	}

	for (AArenaPlayerStart* PlayerStart : TeamStarts)
	{
		if (PlayerStart->IsClaimed())
		{
			if (FallbackPlayerStart == nullptr)
			{
				FallbackPlayerStart = PlayerStart;
			}
			
			if (PlayerStart->GetLocationOccupancy(Player) < EArenaPlayerStartLocationOccupancy::Full)
			{
				if (BestPlayerStart == nullptr)
				{
					BestPlayerStart = PlayerStart;
				}
				else if (PlayerStart->GetLocationOccupancy(Player) < BestPlayerStart->GetLocationOccupancy(Player))
				{
					BestPlayerStart = PlayerStart;
				}
			}
		}
		else
		{
			BestPlayerStart = PlayerStart;
		}
	}

	if (BestPlayerStart)
	{
		return BestPlayerStart;
	}

	return FallbackPlayerStart;
}

void UTDM_PlayerSpawningManagerComponent::OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation)
{
	Super::OnFinishRestartPlayer(Player, StartRotation);
}
