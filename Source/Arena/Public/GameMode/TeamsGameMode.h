// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/ArenaGameMode.h"
#include "TeamsGameMode.generated.h"

class ABlasterCharacter;

namespace MatchState
{
	extern ARENA_API const FName Cooldown; // Display winner and cooldown timer
}

/**
 * 
 */
UCLASS()
class ARENA_API ATeamsGameMode : public AArenaGameMode
{
	GENERATED_BODY()
public:
	ATeamsGameMode();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match State Duration")
	float AgentChoosingDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match State Duration")
	float MatchDuration = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match State Duration")
	float CooldownDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Classes")
	TSubclassOf<ABlasterCharacter> AttackerCharacterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Classes")
	TSubclassOf<ABlasterCharacter> DefenderCharacterClass;

	float LevelStartingTime = 0.0f;

protected:
	/* Begin AGameMode interface */
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleMatchHasStarted() override;
	virtual void OnMatchStateSet() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	/* End AGameMode interface */

private:
	float CountdownTime = 0.0f;
};
