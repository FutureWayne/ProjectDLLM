// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArenaHitFeedbackType.generated.h"

/** Server-confirmed outcome of damage dealt to an enemy. */
UENUM(BlueprintType)
enum class EArenaHitFeedbackType : uint8
{
	Hit,
	Kill,
	InstantKill
};
