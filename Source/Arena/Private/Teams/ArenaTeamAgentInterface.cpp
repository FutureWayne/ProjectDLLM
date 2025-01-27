// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamAgentInterface.h"

#include "ArenaLogChannel.h"
#include "Teams/ArenaTeamSubsystem.h"

UArenaTeamAgentInterface::UArenaTeamAgentInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void IArenaTeamAgentInterface::ConditionalBroadcastTeamChanged(TScriptInterface<IArenaTeamAgentInterface> This,
	FGenericTeamId OldTeamId, FGenericTeamId NewTeamId)
{
	if (OldTeamId != NewTeamId)
	{
		const int32 OldTeamIndex = GenericTeamIdToInteger(OldTeamId);
		const int32 NewTeamIndex = GenericTeamIdToInteger(NewTeamId);

		
		UObject* ThisObj = This.GetObject();
		UE_LOG(LogArenaTeams, Verbose, TEXT("[%s] %s assigned team %d"), *GetClientServerContextString(ThisObj), *GetPathNameSafe(ThisObj), NewTeamIndex);

		This.GetInterface()->GetTeamChangedDelegateChecked().Broadcast(ThisObj, OldTeamIndex, NewTeamIndex);
	}
}
