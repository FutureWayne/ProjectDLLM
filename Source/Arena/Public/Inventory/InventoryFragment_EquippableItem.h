// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_EquippableItem.generated.h"

class UArenaEquipmentDefinition;
/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_EquippableItem : public UArenaInventoryItemFragment
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category=Arena)
	TSubclassOf<UArenaEquipmentDefinition> EquipmentDefinition;
};
