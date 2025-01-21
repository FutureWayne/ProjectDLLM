// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamAgentInterface.h"

UArenaTeamAgentInterface::UArenaTeamAgentInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void IArenaTeamAgentInterface::ConditionalBroadcastTeamChanged(TScriptInterface<IArenaTeamAgentInterface> This,
	FGenericTeamId OldTeamID, FGenericTeamId NewTeamID)
{
	if (OldTeamID != NewTeamID)
	{
		const ETeam OldTeam = GenericTeamIdToTeam(OldTeamID);
		const ETeam NewTeam = GenericTeamIdToTeam(NewTeamID);

		UObject* ThisObj = This.GetObject();

		This.GetInterface()->GetTeamChangedDelegateChecked().Broadcast(ThisObj, OldTeam, NewTeam);
	}
}
