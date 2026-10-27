// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaScoreboardEditorLibrary.generated.h"

class UBlueprint;

UCLASS()
class UArenaScoreboardEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Replaces the stat widget's unowned async node with its scoped native subscription. */
	UFUNCTION(BlueprintCallable, Category = "Arena|Editor")
	static bool MigrateScoreboardStat(UBlueprint* Blueprint);
};
