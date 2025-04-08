// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

#include "ArenaNotificationMessage.generated.h"

class UObject;

ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Arena_AddNotification_Message);

class APlayerState;

// A message destined for a transient log (e.g., an elimination feed or inventory pickup stream)
USTRUCT(BlueprintType)
struct ARENA_API FArenaNotificationMessage
{
	GENERATED_BODY()

	// The destination channel
	UPROPERTY(BlueprintReadWrite, Category=Notification)
	FGameplayTag TargetChannel;

	// The target player (if none is set then it will display for all local players)
	UPROPERTY(BlueprintReadWrite, Category=Notification)
	TObjectPtr<APlayerState> TargetPlayer = nullptr;

	// The message to display
	UPROPERTY(BlueprintReadWrite, Category=Notification)
	FText PayloadMessage;

	// Extra payload specific to the target channel (e.g., a style or definition asset)
	UPROPERTY(BlueprintReadWrite, Category=Notification)
	FGameplayTag PayloadTag;

	// Extra payload specific to the target channel (e.g., a style or definition asset)
	UPROPERTY(BlueprintReadWrite, Category=Notification)
	TObjectPtr<UObject> PayloadObject = nullptr;
};
