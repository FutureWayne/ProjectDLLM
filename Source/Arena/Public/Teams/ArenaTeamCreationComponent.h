// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "ArenaTeamCreationComponent.generated.h"

class AArenaPlayerState;
class AArenaTeamInfo;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaTeamCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UArenaTeamCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

protected:
	// TODO: Use a map to store the team id and the display asset
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TArray<uint8> TeamsToCreate;

	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TSubclassOf<AArenaTeamInfo> TeamInfoClass;
	
#if WITH_SERVER_CODE
protected:
	virtual void ServerCreateTeams();
	virtual void ServerAssignPlayersToTeams();

	/** Sets the team ID for the given player state. Spectator-only player states will be stripped of any team association. */
	virtual void ServerChooseTeamForPlayer(AArenaPlayerState* PS);

private:
	void OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer);
	void ServerCreateTeam(int32 TeamId);

	/** returns the Team ID with the fewest active players, or INDEX_NONE if there are no valid teams */
	int32 GetLeastPopulatedTeamID() const;
#endif
};
