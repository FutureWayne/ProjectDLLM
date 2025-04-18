// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaDevelopmentStatics.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UArenaDevelopmentStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Should game logic skip directly to gameplay (skipping any match warmup / waiting for players / etc... aspects)
	// Will always return false except when playing in the editor and bTestFullGameFlowInPIE (in Lyra Developer Settings) is false
	UFUNCTION(BlueprintCallable, Category="Lyra")
	static bool ShouldSkipDirectlyToGameplay();
};
