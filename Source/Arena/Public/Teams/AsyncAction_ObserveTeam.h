// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "AsyncAction_ObserveTeam.generated.h"

enum class ETeam : uint8;
class IArenaTeamAgentInterface;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTeamObservedAsyncDelegate, bool, bTeamSet, ETeam, TeamId);

/**
 * 
 */
UCLASS()
class ARENA_API UAsyncAction_ObserveTeam : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	UAsyncAction_ObserveTeam(const FObjectInitializer& ObjectInitializer);
	
	// Watches for team changes on the specified team agent
	//  - It will fire once immediately to give the current team assignment
	//  - For anything that can ever belong to a team (implements IArenaTeamAgentInterface),
	//    it will also listen for team assignment changes in the future
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", Keywords="Watch"))
	static UAsyncAction_ObserveTeam* ObserveTeam(UObject* TeamAgent);

	//~UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
	//~End of UBlueprintAsyncActionBase interface

public:
	// Called when the team is set or changed
	UPROPERTY(BlueprintAssignable)
	FTeamObservedAsyncDelegate OnTeamChanged;

private:
	// Watches for team changes on the specified team actor
	static UAsyncAction_ObserveTeam* InternalObserveTeamChanges(TScriptInterface<IArenaTeamAgentInterface> TeamActor);

private:
	UFUNCTION()
	void OnWatchedAgentChangedTeam(UObject* TeamAgent, ETeam OldTeam, ETeam NewTeam);

	TWeakInterfacePtr<IArenaTeamAgentInterface> TeamInterfacePtr;
	
	
};
