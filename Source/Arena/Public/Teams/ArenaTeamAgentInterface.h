// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArenaTeamInfo.h"
#include "GenericTeamAgentInterface.h"
#include "UObject/Interface.h"
#include "ArenaTeamAgentInterface.generated.h"

template <typename InterfaceType> class TScriptInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnArenaTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, ETeam, OldTeamID, ETeam, NewTeamID);

inline ETeam GenericTeamIdToTeam(const FGenericTeamId ID)
{
	return (ID == FGenericTeamId::NoTeam) ? ETeam::ET_Max : static_cast<ETeam>(ID.GetId());
}

inline FGenericTeamId TeamToGenericTeamId(const ETeam Team)
{
	return (Team == ETeam::ET_Max) ? FGenericTeamId::NoTeam : FGenericTeamId(static_cast<uint8>(Team));
}

/** Interface for actors which can be associated with teams */
UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UArenaTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_UINTERFACE_BODY()
};
	
class ARENA_API IArenaTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_IINTERFACE_BODY()

	virtual FOnArenaTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() { return nullptr; }

	static void ConditionalBroadcastTeamChanged(TScriptInterface<IArenaTeamAgentInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID);
	
	FOnArenaTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnArenaTeamIndexChangedDelegate* Result = GetOnTeamIndexChangedDelegate();
		check(Result);
		return *Result;
	}
};


