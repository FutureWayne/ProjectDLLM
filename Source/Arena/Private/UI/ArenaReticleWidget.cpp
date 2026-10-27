// Copyright Ludens Studio. All Rights Reserved.

#include "UI/ArenaReticleWidget.h"

void UArenaReticleWidget::ShowEnemyHit(EArenaHitFeedbackType FeedbackType)
{
	switch (FeedbackType)
	{
	case EArenaHitFeedbackType::Hit:
		OnHitFeedback();
		break;
	case EArenaHitFeedbackType::Kill:
		OnKillFeedback();
		break;
	case EArenaHitFeedbackType::InstantKill:
		OnInstantKillFeedback();
		break;
	}
}
