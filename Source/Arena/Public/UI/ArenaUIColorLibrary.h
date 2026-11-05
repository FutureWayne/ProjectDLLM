// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/ArenaUIColorPalette.h"
#include "ArenaUIColorLibrary.generated.h"

/** Read-only access to the shared palette. Does not apply widget or material styling. */
UCLASS()
class ARENA_API UArenaUIColorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Arena|UI|Colors")
	static FArenaFactionPalette GetTeamPalette(int32 TeamId);

	UFUNCTION(BlueprintPure, Category = "Arena|UI|Colors")
	static FLinearColor GetTeamColor(int32 TeamId, EArenaFactionColorRole Role);

	UFUNCTION(BlueprintPure, Category = "Arena|UI|Colors")
	static FArenaSharedPalette GetSharedPalette();

	UFUNCTION(BlueprintPure, Category = "Arena|UI|Colors")
	static FLinearColor GetSharedColor(EArenaSharedColorRole Role);
};
