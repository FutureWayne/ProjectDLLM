// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/ArenaInventoryItemDefinition.h"

#include "Inventory/ArenaInventoryItemInstance.h"
#include "Inventory/ArenaInventoryManagerComponent.h"
#include "Inventory/InventoryFragment_LoadoutItemData.h"

//////////////////////////////////////////////////////////////////////
// UArenaInventoryItemDefinition

UArenaInventoryItemDefinition::UArenaInventoryItemDefinition(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

const UArenaInventoryItemFragment* UArenaInventoryItemDefinition::FindFragmentByClass(
	const TSubclassOf<UArenaInventoryItemFragment>& FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UArenaInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}
