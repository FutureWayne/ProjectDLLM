// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Phases/ArenaGamePhaseAbility.h"
#include "GameState/ArenaMatchFinaleTypes.h"
#include "ArenaGamePhaseAbility_MatchFinale.generated.h"

/**
 * Base of the post-match phase. The ability runs only on the server, so clients learn the finale through what it
 * publishes: one looping GameplayCue per stage (carrying the stage's real-time length in RawMagnitude) and the frozen
 * values on AArenaGameState. Cues added here are removed automatically when the ability ends or is cancelled.
 *
 * The Blueprint owns the stage order, the waits and the slow motion.
 */
UCLASS(Abstract)
class ARENA_API UArenaGamePhaseAbility_MatchFinale : public UArenaGamePhaseAbility
{
	GENERATED_BODY()

protected:
	/** Freezes both team scores and publishes them with the winner (0 = draw, 1 = red, 2 = blue). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Arena|Finale")
	void BeginMatchFinale(int32 WinningTeamId);

	/**
	 * Makes Stage the current stage on every machine.
	 * @return World seconds to wait so the stage lasts its configured real time under the current time dilation.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Arena|Finale")
	float EnterFinaleStage(EArenaMatchFinaleStage Stage);

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Finale")
	FArenaMatchFinaleTiming FinaleTiming;

private:
	FGameplayTag ActiveStageCueTag;
};
