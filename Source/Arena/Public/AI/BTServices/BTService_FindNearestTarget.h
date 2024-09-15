// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FindNearestTarget.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UBTService_FindNearestTarget : public UBTService
{
	GENERATED_BODY()
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "AI")
	FBlackboardKeySelector TargetToFollowSelector;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "AI")
	FBlackboardKeySelector MoveToLocation;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "AI")
	FBlackboardKeySelector DistanceToTargetSelector;
};
