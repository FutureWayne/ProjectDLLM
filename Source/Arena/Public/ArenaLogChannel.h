// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

ARENA_API DECLARE_LOG_CATEGORY_EXTERN(LogArena, Log, All);
ARENA_API DECLARE_LOG_CATEGORY_EXTERN(LogArenaAbilitySystem, Log, All);
ARENA_API DECLARE_LOG_CATEGORY_EXTERN(LogArenaTeams, Log, All);

ARENA_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
