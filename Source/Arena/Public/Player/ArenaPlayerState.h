// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystem/ArenaAbilitySet.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Teams/ArenaTeamInfo.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "ArenaPlayerState.generated.h"

enum class ETeam : uint8;
class UArenaHealthSet;

/**
 * 
 */
UCLASS()
class ARENA_API AArenaPlayerState : public APlayerState, public IAbilitySystemInterface, public IGenericTeamAgentInterface
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

	void AddAbilitySet(const UArenaAbilitySet* AbilitySet);

	// ~IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override { return MyTeamID; }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	// ~ End IGenericTeamAgentInterface

	/** Returns the ETeam of the team the player belongs to. */
	UFUNCTION(BlueprintCallable)
	ETeam GetTeam() const
	{
		return GenericTeamIdToTeam(MyTeamID);
	}

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;
	
private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

private:
	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	FArenaAbilitySet_GrantedHandles AbilitySetHandles;
};
