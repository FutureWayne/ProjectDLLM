#include "Inventory/ArenaInventoryFunctionLibrary.h"

#include "Inventory/ArenaInventoryItemDefinition.h"
#include "Inventory/InventoryFragment_LoadoutItemData.h"


//////////////////////////////////////////////////////////////////////
// UArenaInventoryFunctionLibrary

const UArenaInventoryItemFragment* UArenaInventoryFunctionLibrary::FindItemDefinitionFragment(
	TSubclassOf<UArenaInventoryItemDefinition> ItemDef, TSubclassOf<UArenaInventoryItemFragment> FragmentClass)
{
	if (ItemDef != nullptr && FragmentClass != nullptr)
	{
		return GetDefault<UArenaInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}

const UArenaInventoryItemInstance* UArenaInventoryFunctionLibrary::FindFirstItemInstanceByLoadoutType(
	const UArenaInventoryManagerComponent* InventoryManager, ELoadoutType LoadoutType)
{
	if (!InventoryManager)
	{
		return nullptr;
	}
	
	TArray<UArenaInventoryItemInstance*> InventoryItems = InventoryManager->GetAllItems();

	for (UArenaInventoryItemInstance* ItemInstance : InventoryItems)
	{
		const UInventoryFragment_LoadoutItemData* LoadoutFragment = ItemInstance->FindFragmentByClass<UInventoryFragment_LoadoutItemData>();
		if (LoadoutFragment && LoadoutFragment->ItemLoadoutType == LoadoutType)
		{
			return ItemInstance;
		}
	}
	
	return nullptr;
}