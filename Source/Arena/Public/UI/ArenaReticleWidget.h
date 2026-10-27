// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "UI/ArenaHitFeedbackType.h"
#include "ArenaReticleWidget.generated.h"

/** Routes damage outcomes to presentation events authored in the reticle Blueprint. */
UCLASS()
class ARENA_API UArenaReticleWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void ShowEnemyHit(EArenaHitFeedbackType FeedbackType);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Feedback")
	void OnHitFeedback();

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Feedback")
	void OnKillFeedback();

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Feedback")
	void OnInstantKillFeedback();
};
