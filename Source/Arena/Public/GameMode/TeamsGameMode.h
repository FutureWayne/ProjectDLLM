// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/ArenaGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API ATeamsGameMode : public AArenaGameMode
{
	GENERATED_BODY()
public:
	ATeamsGameMode();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode")
	float AgentChoosingDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode")
	float MatchDuration = 120.f;

	float LevelStartingTime = 0.0f;

protected:
	/* Begin AGameMode interface */
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleMatchHasStarted() override;
	virtual void OnMatchStateSet() override;
	/* End AGameMode interface */

private:
	float CountdownTime = 0.0f;
};
