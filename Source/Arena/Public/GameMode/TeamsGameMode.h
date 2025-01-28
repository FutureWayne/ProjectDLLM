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

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Classes")
	TSubclassOf<ABlasterCharacter> AttackerCharacterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Classes")
	TSubclassOf<ABlasterCharacter> DefenderCharacterClass;

protected:
	/* Begin AGameMode interface */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	/* End AGameMode interface */
};
