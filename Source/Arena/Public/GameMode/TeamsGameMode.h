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
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
protected:
	virtual void HandleMatchHasStarted() override;
};
