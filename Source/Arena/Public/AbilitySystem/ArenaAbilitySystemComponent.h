// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ArenaAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UArenaAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AddCharacterAbilities(const TArray<TSubclassOf<class UGameplayAbility>>& StartupAbilities);

	void AbilityTagHeld(const FGameplayTag& InputTag);
	void AbilityTagPressed(const FGameplayTag& InputTag);
	void AbilityTagReleased(const FGameplayTag& InputTag);
};
