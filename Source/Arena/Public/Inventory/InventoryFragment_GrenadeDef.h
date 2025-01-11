// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_GrenadeDef.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_GrenadeDef : public UArenaInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=GrenadeInfo)
	TSubclassOf<AActor> GrenadeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=GrenadeInfo)
	int32 BurstShotCount = 1;
};
