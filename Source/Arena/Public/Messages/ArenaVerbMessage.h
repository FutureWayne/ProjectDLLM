// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "ArenaVerbMessage.generated.h"

// Represents a generic message of the form Instigator Verb Target (in Context, with Magnitude)
USTRUCT(BlueprintType)
struct FArenaVerbMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTag Verb;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer InstigatorTags;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer TargetTags;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer ContextTags;

	/**
	 * Optional asset describing how the verb happened, e.g. the grenade definition credited for an elimination.
	 * Replicated messages must use a net-addressable (asset) object; spawned actors may already be gone on clients.
	 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<const UObject> ContextObject = nullptr;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	double Magnitude = 1.0;

	// Returns a debug string representation of this message
	ARENA_API FString ToString() const;
};

