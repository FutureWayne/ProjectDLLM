// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArenaTeamInfo.h"
#include "Subsystems/WorldSubsystem.h"
#include "ArenaTeamSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTeam, Log, All);

class AArenaPlayerState;

// Result of comparing the team affiliation for two actors
UENUM(BlueprintType)
enum class EArenaTeamComparison : uint8
{
	// Both actors are members of the same team
	OnSameTeam,

	// The actors are members of opposing teams
	DifferentTeams,

	// One (or both) of the actors was invalid or not part of any team
	InvalidArgument
};

/**
 * 
 */
UCLASS()
class ARENA_API UArenaTeamSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UArenaTeamSubsystem();

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface
	
	// Tries to register a new team
	bool RegisterTeamInfo(AArenaTeamInfo* TeamInfo);

	// Tries to unregister a team
	bool UnregisterTeamInfo(AArenaTeamInfo* TeamInfo);
	
	// Returns the team this object belongs to, or ET_Max if it is not part of a team
	ETeam FindTeamFromObject(const UObject* TestObject) const;

	// Returns the associated player state for this actor, or nullptr if it is not associated with a player
	const AArenaPlayerState* FindPlayerStateFromActor(const AActor* PossibleTeamActor) const;

	// Changes the team associated with this actor if possible
	// Note: This function can only be called on the authority
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(Keywords="Set"))
	bool ChangeTeamForActor(AActor* ActorToChange, const ETeam NewTeam) const;
	
	// Returns the team this object belongs to, or ET_Max if it is not part of a team
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(Keywords="Get"))
	void FindTeamFromActor(const UObject* TestObject, bool& bIsPartOfTeam, ETeam& Team) const;

	// Compare the teams of two actors and returns a value indicating if they are on same teams, different teams, or one/both are invalid
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(ExpandEnumAsExecs=ReturnValue))
	EArenaTeamComparison CompareTeams(const UObject* A, const UObject* B, ETeam& TeamA, ETeam& TeamB) const;

	// Returns the team this object belongs to, or INDEX_NONE if it is not part of a team
	UFUNCTION(BlueprintCallable, Category=Teams, meta=(Keywords="GetTeamFromObject", DefaultToSelf="Agent", AdvancedDisplay="bLogIfNotSet"))
	static void FindTeamFromObject(const UObject* Agent, bool& bIsPartOfTeam, ETeam& TeamId, FLinearColor& TeamColor, bool bLogIfNotSet = false);
	
	// Returns the team color for the actor
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Teams, meta=(Keywords="Get"))
	FLinearColor FindTeamColorFromActor(const UObject* TestObject) const;

	// Compare the teams of two actors and returns a value indicating if they are on same teams, different teams, or one/both are invalid
	EArenaTeamComparison CompareTeams(const UObject* A, const UObject* B) const;

	// Returns true if the instigator can damage the target, taking into account the friendly fire settings
	bool CanCauseDamage(const UObject* Instigator, const UObject* Target, bool bAllowDamageToSelf = true) const;

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddTeamTagStack(ETeam TeamId, FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveTeamTagStack(ETeam TeamId, FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetTeamTagStackCount(ETeam TeamId, FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool TeamHasTag(ETeam TeamId, FGameplayTag Tag) const;

private:
	TMap<ETeam, TObjectPtr<AArenaTeamInfo>> TeamMap;

	static const TMap<ETeam, FLinearColor> TeamColorMap;
};
