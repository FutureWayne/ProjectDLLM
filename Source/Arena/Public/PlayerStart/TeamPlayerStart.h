// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TeamPlayerStart.generated.h"

enum class ETeam : uint8;
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
