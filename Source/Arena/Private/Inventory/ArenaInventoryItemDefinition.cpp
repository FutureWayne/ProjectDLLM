// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/ArenaInventoryItemDefinition.h"

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

