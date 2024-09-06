// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace ArenaGameplayTags
{
	ARENA_API FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death);

	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look);
	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Equip);
	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Drop);
	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);
	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Aim);
	ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Fire);
}
