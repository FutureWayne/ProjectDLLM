// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ArenaAbilitySet.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Teams/ArenaTeamAgentInterface.h"
#include "Teams/ArenaTeamInfo.h"
#include "ArenaPlayerState.generated.h"

class UArenaCombatSet;
class UArenaHealthSet;

enum class ETeam : uint8;

/**
 * 
 */
UCLASS()
class ARENA_API AArenaPlayerState : public APlayerState, public IAbilitySystemInterface, public IArenaTeamAgentInterface
{
	GENERATED_BODY()

public:
	AArenaPlayerState();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Arena|PlayerState")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const { return AbilitySystemComponent; };
	
	UArenaHealthSet* GetArenaHealthSet() const { return ArenaHealthSet; }

	UArenaCombatSet* GetArenaCombatSet() const { return ArenaCombatSet; }

	void AddAbilitySet(const UArenaAbilitySet* AbilitySet);

	//~IArenaTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnArenaTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IArenaTeamAgentInterface interface

	/** Returns the ETeam of the team the player belongs to. */
	UFUNCTION(BlueprintCallable)
	ETeam GetTeam() const
	{
		return GenericTeamIdToTeam(MyTeamID);
	}

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool HasStatTag(FGameplayTag Tag) const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArenaCombatSet> ArenaCombatSet;
	
private:
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

private:
	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	UPROPERTY()
	FOnArenaTeamIndexChangedDelegate OnTeamChangedDelegate;

	FArenaAbilitySet_GrantedHandles AbilitySetHandles;
};
