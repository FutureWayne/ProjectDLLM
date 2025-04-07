// Copyright Ludens Studio. All Rights Reserved.

#pragma once
#include "ArenaInventoryItemInstance.h"
#include "ArenaInventoryManagerComponent.h"

#include "ArenaInventoryFunctionLibrary.generated.h"

class UArenaInventoryItemDefinition;
class UArenaInventoryItemFragment;

UCLASS()
class ARENA_API UArenaInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UArenaInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UArenaInventoryItemDefinition> ItemDef, TSubclassOf<UArenaInventoryItemFragment> FragmentClass);

	UFUNCTION(BlueprintCallable)
	static const UArenaInventoryItemInstance* FindFirstItemInstanceByLoadoutType(
		const UArenaInventoryManagerComponent* InventoryManager, FGameplayTag LoadoutTypeTag);
};
