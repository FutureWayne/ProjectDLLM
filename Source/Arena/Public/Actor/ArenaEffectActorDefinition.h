// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "ArenaEffectActorDefinition.generated.h"

/** Shared configuration contract for effects spawned by grenades or other gameplay systems. */
UCLASS(Abstract, BlueprintType)
class ARENA_API UArenaEffectActorDefinition : public UDataAsset
{
	GENERATED_BODY()
};
