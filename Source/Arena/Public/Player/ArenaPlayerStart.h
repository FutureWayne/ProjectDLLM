// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerStart.h"
#include "ArenaPlayerStart.generated.h"

enum class EArenaPlayerStartLocationOccupancy
{
	Empty,
	Partial,
	Full
};

/**
 * 
 */
UCLASS(Config = Game)
class ARENA_API AArenaPlayerStart : public APlayerStart
{
	GENERATED_BODY()
	
public:
	AArenaPlayerStart(const FObjectInitializer& ObjectInitializer);

	const FGameplayTagContainer& GetStartPointTags() const { return StartPointTags; }

	int32 GetTeamId() const { return TeamId; }

	EArenaPlayerStartLocationOccupancy GetLocationOccupancy(AController* const ControllerPawnToFit) const;

	/** Did this player start get claimed by a controller already? */
	bool IsClaimed() const;
	
	/** If this PlayerStart was not claimed, claim it for ClaimingController */
	bool TryClaim(AController* OccupyingController);

protected:
	/** Check if this PlayerStart is still claimed */
	void CheckUnclaimed();

	/** The controller that claimed this PlayerStart */
	UPROPERTY(Transient)
	TObjectPtr<AController> ClaimingController = nullptr;

	/** Interval in which we'll check if this player start is not colliding with anyone anymore */
	UPROPERTY(EditDefaultsOnly, Category = "Player Start Claiming")
	float ExpirationCheckInterval = 1.f;
	
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer StartPointTags;

	UPROPERTY(EditAnywhere)
	int32 TeamId = 0;

	FTimerHandle ExpirationTimerHandle;
};
