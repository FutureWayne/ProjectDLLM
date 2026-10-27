// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Player/ArenaPlayerController.h"
#include "ArenaHitFeedbackTestController.generated.h"

UCLASS(Transient, NotBlueprintable)
class AArenaHitFeedbackTestController : public AArenaPlayerController
{
	GENERATED_BODY()

public:
	virtual void ClientNotifyEnemyHit_Implementation(EArenaHitFeedbackType FeedbackType) override;

	TArray<EArenaHitFeedbackType> ReceivedFeedback;

protected:
	virtual void SetupInputComponent() override;
};
