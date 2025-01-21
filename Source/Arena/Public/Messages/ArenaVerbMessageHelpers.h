// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaVerbMessageHelpers.generated.h"

struct FArenaVerbMessage;
struct FGameplayCueParameters;

class APlayerState;
class APlayerController;
class UObject;


/**
 * 
 */
UCLASS()
class ARENA_API UArenaVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Arena")
	static APlayerState* GetPlayerStateFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "Arena")
	static APlayerController* GetPlayerControllerFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "Arena")
	static FGameplayCueParameters VerbMessageToCueParameters(const FArenaVerbMessage& Message);

	UFUNCTION(BlueprintCallable, Category = "Arena")
	static FArenaVerbMessage CueParametersToVerbMessage(const FGameplayCueParameters& Params);
};
