// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "ArenaPlayerState.generated.h"

class UArenaHealthSet;

/**
 * 
 */
UCLASS()
class ARENA_API AArenaPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AArenaPlayerState();

	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Arena|PlayerState")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const { return AbilitySystemComponent; };
	
	UArenaHealthSet* GetArenaHealthSet() const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;
};
