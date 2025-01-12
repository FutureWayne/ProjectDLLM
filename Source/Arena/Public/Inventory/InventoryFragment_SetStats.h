// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Inventory/ArenaInventoryItemDefinition.h"

#include "InventoryFragment_SetStats.generated.h"

class UArenaInventoryItemInstance;

/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_SetStats : public UArenaInventoryItemFragment
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UArenaInventoryItemInstance* Instance, AActor* OwnerActor) override;

	int32 GetItemStatByTag(FGameplayTag Tag) const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TMap<FGameplayTag, int32> InitialItemStats;

};
