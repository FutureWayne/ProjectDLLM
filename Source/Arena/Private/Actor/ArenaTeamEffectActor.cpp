// Copyright Ludens Studio. All Rights Reserved.


#include "Actor/ArenaTeamEffectActor.h"

#include "Net/UnrealNetwork.h"


void AArenaTeamEffectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaTeamEffectActor, MyTeamID);
}

void AArenaTeamEffectActor::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	MyTeamID = NewTeamID;
}
