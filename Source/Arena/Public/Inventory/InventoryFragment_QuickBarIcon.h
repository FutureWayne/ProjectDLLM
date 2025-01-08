// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Inventory/ArenaInventoryItemDefinition.h"
#include "Styling/SlateBrush.h"

#include "InventoryFragment_QuickBarIcon.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_QuickBarIcon : public UArenaInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateBrush Brush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FText DisplayNameWhenEquipped;
};
