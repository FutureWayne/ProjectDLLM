// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ArenaGameplayAbility.generated.h"

/**
 * EArenaAbilityActivationPolicy
 *
 *	Defines how an ability is meant to activate.
 */
UENUM(BlueprintType)
enum class EArenaAbilityActivationPolicy : uint8
{
	// Try to activate the ability when the input is triggered.
	OnInputTriggered,

	// Continually try to activate the ability while the input is active.
	WhileInputActive,

	// Try to activate the ability when an avatar is assigned.
	OnSpawn
};

/**
 * 
 */
UCLASS()
class ARENA_API UArenaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	FGameplayTag StartupInputTag;

	EArenaAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|AbilitySystem")
	EArenaAbilityActivationPolicy ActivationPolicy;
};
