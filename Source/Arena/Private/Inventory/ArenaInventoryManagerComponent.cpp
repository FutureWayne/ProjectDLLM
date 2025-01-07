// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/ArenaInventoryManagerComponent.h"

#include "NativeGameplayTags.h"
#include "Engine/ActorChannel.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Inventory_Message_StackChanged, "Arena.Inventory.Message.StackChanged");

//////////////////////////////////////////////////////////////////////
// FArenaInventoryEntry

FString FArenaInventoryEntry::GetDebugString() const
{
	TSubclassOf<UArenaInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

//////////////////////////////////////////////////////////////////////
// FArenaInventoryList

TArray<UArenaInventoryItemInstance*> FArenaInventoryList::GetAllItems() const
{
	TArray<UArenaInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FArenaInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr)
		{
			Results.Add(Entry.Instance);
		}
	}

	return Results;
}

void FArenaInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FArenaInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/Stack.StackCount, /*NewCount=*/0);
		Stack.LastObserverCount = 0;
	}
}

void FArenaInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FArenaInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/0, /*NewCount=*/Stack.StackCount);
		Stack.LastObserverCount = Stack.StackCount;
	}
}

void FArenaInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FArenaInventoryEntry& Stack = Entries[Index];
		check(Stack.LastObserverCount != INDEX_NONE);
		BroadcastChangeMessage(Stack, /*OldCount=*/Stack.LastObserverCount, /*NewCount=*/Stack.StackCount);
		Stack.LastObserverCount = Stack.StackCount;
	}
}

UArenaInventoryItemInstance* FArenaInventoryList::AddEntry(const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef,
                                                           const int32 StackCount)
{
	UArenaInventoryItemInstance* Result = nullptr;

	check(ItemDef != nullptr);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FArenaInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UArenaInventoryItemInstance>(OwnerComponent->GetOwner());
	NewEntry.Instance->SetItemDef(ItemDef);
	const UArenaInventoryItemDefinition* ItemCDO = GetDefault<UArenaInventoryItemDefinition>(ItemDef);
	for (const UArenaInventoryItemFragment* Fragment : ItemCDO->Fragments)
	{
		if (Fragment != nullptr)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}

	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	MarkItemDirty(NewEntry);

	return Result;
}

void FArenaInventoryList::AddEntry(UArenaInventoryItemInstance* ItemInstance)
{
	unimplemented();
}

void FArenaInventoryList::RemoveEntry(UArenaInventoryItemInstance* ItemInstance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FArenaInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == ItemInstance)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

void FArenaInventoryList::BroadcastChangeMessage(FArenaInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
	
}

//////////////////////////////////////////////////////////////////////
// ULyraInventoryManagerComponent

UArenaInventoryManagerComponent::UArenaInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
}

void UArenaInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

bool UArenaInventoryManagerComponent::CanAddItemDefinition(TSubclassOf<UArenaInventoryItemDefinition>& ItemDef,
	const int32 StackCount) const
{
	//@TODO: Add support for stack limit / uniqueness checks / etc...
	return true;
}

UArenaInventoryItemInstance* UArenaInventoryManagerComponent::AddItemDefinition(
	const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef, const int32 StackCount)
{
	UArenaInventoryItemInstance* Result = nullptr;
	if (ItemDef != nullptr)
	{
		Result = InventoryList.AddEntry(ItemDef, StackCount);

		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
		{
			AddReplicatedSubObject(Result);
		}
	}

	return Result;
}

void UArenaInventoryManagerComponent::AddItemInstance(UArenaInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

void UArenaInventoryManagerComponent::RemoveItemInstance(UArenaInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}
}


TArray<UArenaInventoryItemInstance*> UArenaInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}

UArenaInventoryItemInstance* UArenaInventoryManagerComponent::FindFirstItemStackByDefinition(
	const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef) const
{
	for (const FArenaInventoryEntry& Entry : InventoryList.Entries)
	{
		UArenaInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

int32 UArenaInventoryManagerComponent::GetTotalItemCountByDefinition(
	const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef) const
{
	int32 TotalCount = 0;

	for (const FArenaInventoryEntry& Entry : InventoryList.Entries)
	{
		UArenaInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				++TotalCount;
			}
		}
	}

	return TotalCount;
}

bool UArenaInventoryManagerComponent::ConsumeItemsByDefinition(const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef,
                                                               const int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (UArenaInventoryItemInstance* Instance = FindFirstItemStackByDefinition(ItemDef))
		{
			InventoryList.RemoveEntry(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

void UArenaInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing UArenaInventoryItemInstance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FArenaInventoryEntry& Entry : InventoryList.Entries)
		{
			UArenaInventoryItemInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}


bool UArenaInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	for (FArenaInventoryEntry& Entry : InventoryList.Entries)
	{
		UArenaInventoryItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}
	
	return Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
}

