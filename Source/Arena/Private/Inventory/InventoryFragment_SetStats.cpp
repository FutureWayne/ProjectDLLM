// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/InventoryFragment_SetStats.h"

#include "Inventory/ArenaInventoryItemInstance.h"


void UInventoryFragment_SetStats::OnInstanceCreated(UArenaInventoryItemInstance* Instance) const
{
	for (const auto& KVP : InitialItemStats)
	{
		Instance->AddStatTagStack(KVP.Key, KVP.Value);
	}
}

int32 UInventoryFragment_SetStats::GetItemStatByTag(const FGameplayTag Tag) const
{
	if (const int32* StatPtr = InitialItemStats.Find(Tag))
	{
		return *StatPtr;
	}

	return 0;
}
