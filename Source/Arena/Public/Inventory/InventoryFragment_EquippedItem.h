// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_EquippedItem.generated.h"

class UArenaEquipmentDefinition;
/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_EquippedItem : public UArenaInventoryItemFragment
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category=Arena)
	TSubclassOf<UArenaEquipmentDefinition> EquipmentDefinition;
	
	
};
