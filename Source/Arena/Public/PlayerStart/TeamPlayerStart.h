// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Player/Team.h"
#include "TeamPlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Team", Meta = (AllowPrivateAccess = "true"))
	ETeam Team;
};
