// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/ArenaGameMode.h"
#include "TeamsGameMode.generated.h"

class AArenaCharacter;

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

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Classes")
	TSubclassOf<AArenaCharacter> AttackerCharacterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Classes")
	TSubclassOf<AArenaCharacter> DefenderCharacterClass;

protected:
	/* Begin AGameMode interface */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	/* End AGameMode interface */
};
