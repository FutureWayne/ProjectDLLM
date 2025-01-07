// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/ArenaInventoryItemInstance.h"

#include "GameplayTagContainer.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"


UArenaInventoryItemInstance::UArenaInventoryItemInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UArenaInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
}

void UArenaInventoryItemInstance::AddStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void UArenaInventoryItemInstance::RemoveStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 UArenaInventoryItemInstance::GetStatTagStackCount(const FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool UArenaInventoryItemInstance::HasStatTag(const FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

const UArenaInventoryItemFragment* UArenaInventoryItemInstance::FindFragmentByClass(
	const TSubclassOf<UArenaInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UArenaInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}

void UArenaInventoryItemInstance::SetItemDef(const TSubclassOf<UArenaInventoryItemDefinition>& InItemDef)
{
	ItemDef = InItemDef;
}
