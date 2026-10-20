// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;
class AActor;
struct FHitResult;

namespace ArenaGroundArea
{
	// Pawns (including vehicles), projectiles and effect/deployment actors never support ground areas.
	bool ShouldIgnore(const AActor* Actor);

	bool Trace(const UWorld* World, FVector Start, FVector End, const AActor* Ignore, FHitResult& Hit);
}
