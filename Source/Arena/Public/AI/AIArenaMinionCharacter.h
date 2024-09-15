// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ArenaMinion.h"
#include <BehaviorTree/BTNode.h>
#include "AIArenaMinionCharacter.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AAIArenaMinionCharacter : public AArenaMinion
{
	GENERATED_BODY()
public:
	AAIArenaMinionCharacter();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;
};
