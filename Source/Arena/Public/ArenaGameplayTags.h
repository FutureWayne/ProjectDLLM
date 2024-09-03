// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace ArenaGameplayTags
{
	ARENA_API FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death);
}
