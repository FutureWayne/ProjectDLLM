// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ArenaCharacterBase.h"
#include "ArenaCharacter.generated.h"

class AArenaPlayerState;
/**
 * 
 */
UCLASS()
class ARENA_API AArenaCharacter : public AArenaCharacterBase
{
	GENERATED_BODY()

public:
	AArenaCharacter();

protected:
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;

	// Begins the death sequence for the character (disables collision, disables movement, etc...)
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	// Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	void DisableMovementAndCollision();
	void DestroyDueToDeath();

	// Called when the death sequence for the character has completed
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();

protected:
	TObjectPtr<AArenaPlayerState> ArenaPlayerState;

	virtual void OnPlayerStateInitialized();

private:
	void InitAbilityActorInfo();
};
