// Copyright Ludens Studio. All Rights Reserved.


#include "GameState/ArenaGameState.h"

AActor* AArenaGameState::GetAttackTeamTarget()
{
	return AttackTeamTarget;
}

void AArenaGameState::SetAttackTeamTarget(AActor* Target)
{
	AttackTeamTarget = Target;
}
