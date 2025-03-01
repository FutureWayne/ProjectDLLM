// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_GrenadeDef.generated.h"

class UArenaGrenadeDefinitionData;


/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_GrenadeDef : public UArenaInventoryItemFragment
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly)
	UArenaGrenadeDefinitionData* GrenadeDefinitionData;

public:
	UFUNCTION(BlueprintCallable)
	UArenaGrenadeDefinitionData* GetGrenadeDefinitionData() const;
};
