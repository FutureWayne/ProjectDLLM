// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Team.h"
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
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Arena|PlayerState")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const { return AbilitySystemComponent; };
	
	UArenaHealthSet* GetArenaHealthSet() const;

	void SetTeam(const ETeam NewTeam);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_Neutral;

	UFUNCTION()
	void OnRep_Team();

public:
	UFUNCTION(BlueprintCallable)
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
