// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "ArenaGamePhaseSubsystem.generated.h"

class UArenaGamePhaseAbility;

DECLARE_DYNAMIC_DELEGATE_OneParam(FArenaGamePhaseDynamicDelegate, const UArenaGamePhaseAbility*, Phase);
DECLARE_DELEGATE_OneParam(FArenaGamePhaseDelegate, const UArenaGamePhaseAbility* Phase);

DECLARE_DYNAMIC_DELEGATE_OneParam(FArenaGamePhaseTagDynamicDelegate, const FGameplayTag&, PhaseTag);
DECLARE_DELEGATE_OneParam(FArenaGamePhaseTagDelegate, const FGameplayTag& PhaseTag);

// Match rule for message receivers
UENUM(BlueprintType)
enum class EPhaseTagMatchType : uint8
{
	// An exact match will only receive messages with exactly the same channel
	// (e.g., registering for "A.B" will match a broadcast of A.B but not A.B.C)
	ExactMatch,

	// A partial match will receive any messages rooted in the same channel
	// (e.g., registering for "A.B" will match a broadcast of A.B as well as A.B.C)
	PartialMatch
};

/**
 * 
 */
UCLASS()
class ARENA_API UArenaGamePhaseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UArenaGamePhaseSubsystem();

	virtual void PostInitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	void StartPhase(const TSubclassOf<UArenaGamePhaseAbility>& PhaseAbility, const FArenaGamePhaseDelegate& PhaseEndedCallback = FArenaGamePhaseDelegate());

	void WhenPhaseStartsOrIsActive(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, const FArenaGamePhaseTagDelegate& WhenPhaseActive);
	void WhenPhaseEnds(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, const FArenaGamePhaseTagDelegate& WhenPhaseEnd);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintPure = false, meta = (AutoCreateRefTerm = "PhaseTag"))
	bool IsPhaseActive(const FGameplayTag& PhaseTag) const;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Phases", meta = (DisplayName = "Start Phase", AutoCreateRefTerm = "PhaseEndedDelegate"))
	void K2_StartPhase(TSubclassOf<UArenaGamePhaseAbility> PhaseAbility, const FArenaGamePhaseDynamicDelegate& PhaseEndedDelegate);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Phases", meta = (DisplayName = "When Phase Starts Or Is Active", AutoCreateRefTerm = "WhenPhaseActive"))
	void K2_WhenPhaseStartsOrIsActive(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, FArenaGamePhaseTagDynamicDelegate WhenPhaseActive);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Phases", meta = (DisplayName = "When Phase Ends", AutoCreateRefTerm = "WhenPhaseEnd"))
	void K2_WhenPhaseEnds(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, FArenaGamePhaseTagDynamicDelegate WhenPhaseEnd);

	void OnBeginPhase(const UArenaGamePhaseAbility* PhaseAbility, const FGameplayAbilitySpecHandle& PhaseAbilityHandle);
	void OnEndPhase(const UArenaGamePhaseAbility* PhaseAbility, const FGameplayAbilitySpecHandle& PhaseAbilityHandle);

private:
	struct FArenaGamePhaseEntry
	{
	public:
		FGameplayTag PhaseTag;
		FArenaGamePhaseDelegate PhaseEndedCallback;
	};

	TMap<FGameplayAbilitySpecHandle, FArenaGamePhaseEntry> ActivePhaseMap;

	struct FPhaseObserver
	{
	public:
		bool IsMatch(const FGameplayTag& ComparePhaseTag) const;

		FGameplayTag PhaseTag;
		EPhaseTagMatchType MatchType = EPhaseTagMatchType::ExactMatch;
		FArenaGamePhaseTagDelegate PhaseCallback;
	};

	TArray<FPhaseObserver> PhaseStartObservers;
	TArray<FPhaseObserver> PhaseEndObservers;

	friend class UArenaGamePhaseAbility;
};
