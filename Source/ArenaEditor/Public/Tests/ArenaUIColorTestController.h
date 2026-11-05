// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Player/ArenaPlayerController.h"
#include "ArenaUIColorTestController.generated.h"

/** Production team notifications without experience-specific input assets. */
UCLASS(Transient)
class AArenaUIColorTestController : public AArenaPlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override
	{
		ACommonPlayerController::BeginPlay();
	}

	virtual void SetupInputComponent() override
	{
		ACommonPlayerController::SetupInputComponent();
	}
};
