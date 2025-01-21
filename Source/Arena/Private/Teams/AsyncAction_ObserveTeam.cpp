// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/AsyncAction_ObserveTeam.h"

#include "Teams/ArenaTeamAgentInterface.h"
#include "Teams/ArenaTeamInfo.h"

UAsyncAction_ObserveTeam::UAsyncAction_ObserveTeam(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAsyncAction_ObserveTeam* UAsyncAction_ObserveTeam::ObserveTeam(UObject* TeamAgent)
{
	return InternalObserveTeamChanges(TeamAgent);
}

UAsyncAction_ObserveTeam* UAsyncAction_ObserveTeam::InternalObserveTeamChanges(
	TScriptInterface<IArenaTeamAgentInterface> TeamActor)
{
	UAsyncAction_ObserveTeam* Action = nullptr;

	if (TeamActor != nullptr)
	{
		Action = NewObject<UAsyncAction_ObserveTeam>();
		Action->TeamInterfacePtr = TeamActor;
		Action->RegisterWithGameInstance(TeamActor.GetObject());
	}

	return Action;
}

void UAsyncAction_ObserveTeam::Activate()
{
	bool bCouldSucceed = false;
	ETeam CurrentTeam = ETeam::ET_Max;

	if (IArenaTeamAgentInterface* TeamAgentInterface = TeamInterfacePtr.Get())
	{
		CurrentTeam = GenericTeamIdToTeam(TeamAgentInterface->GetGenericTeamId());
		
		TeamAgentInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnWatchedAgentChangedTeam);

		bCouldSucceed = true;
	}

	OnTeamChanged.Broadcast(CurrentTeam != ETeam::ET_Max, CurrentTeam);

	if (!bCouldSucceed)
	{
		SetReadyToDestroy();
	}
}

void UAsyncAction_ObserveTeam::SetReadyToDestroy()
{
	Super::SetReadyToDestroy();

	if (IArenaTeamAgentInterface* TeamInterface = TeamInterfacePtr.Get())
	{
		TeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
	}
}


void UAsyncAction_ObserveTeam::OnWatchedAgentChangedTeam(UObject* TeamAgent, ETeam OldTeam, ETeam NewTeam)
{
	OnTeamChanged.Broadcast(NewTeam != ETeam::ET_Max, NewTeam);
}
