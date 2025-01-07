// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ArenaInventoryManagerComponent.generated.h"

struct FArenaInventoryList;
class UArenaInventoryItemDefinition;
class UArenaInventoryManagerComponent;
class UArenaInventoryItemInstance;

/** A message when an item is added to the inventory */
USTRUCT(BlueprintType)
struct FArenaInventoryChangeMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<UArenaInventoryItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};

/** A single entry in an inventory */
USTRUCT(BlueprintType)
struct FArenaInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FArenaInventoryEntry()
	{}

	FString GetDebugString() const;

private:
	friend FArenaInventoryList;
	friend UArenaInventoryManagerComponent;

	UPROPERTY()
	TObjectPtr<UArenaInventoryItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY(NotReplicated)
	int32 LastObserverCount = INDEX_NONE;
};

/** List of inventory items */
USTRUCT(BlueprintType)
struct FArenaInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FArenaInventoryList()
		: OwnerComponent(nullptr)
	{
	}

	FArenaInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	TArray<UArenaInventoryItemInstance*> GetAllItems() const;

public:

	// ~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	// ~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FArenaInventoryEntry, FArenaInventoryList>(Entries, DeltaParms, *this);
	}

	UArenaInventoryItemInstance* AddEntry(const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef, int32 StackCount);
	void AddEntry(UArenaInventoryItemInstance* ItemInstance);
	void RemoveEntry(UArenaInventoryItemInstance* ItemInstance);

private:
	void BroadcastChangeMessage(FArenaInventoryEntry& Entry, int32 OldCount, int32 NewCount);

private:
	friend UArenaInventoryManagerComponent;

private:
	UPROPERTY()
	TArray<FArenaInventoryEntry> Entries;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent = nullptr;
};


UCLASS(BlueprintType)
class ARENA_API UArenaInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UArenaInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	bool CanAddItemDefinition(TSubclassOf<class UArenaInventoryItemDefinition>& ItemDef, const int32 StackCount = 1) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UArenaInventoryItemInstance* AddItemDefinition(const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef, const int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddItemInstance(UArenaInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveItemInstance(UArenaInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure=false)
	TArray<UArenaInventoryItemInstance*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	UArenaInventoryItemInstance* FindFirstItemStackByDefinition(const TSubclassOf<UArenaInventoryItemDefinition>& ItemDef) const;

	int32 GetTotalItemCountByDefinition(const TSubclassOf<class UArenaInventoryItemDefinition>& ItemDef) const;
	bool ConsumeItemsByDefinition(const TSubclassOf<class UArenaInventoryItemDefinition>& ItemDef, int32 NumToConsume);

	// ~UObject interface
	virtual void ReadyForReplication() override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	// ~End of UObject interface

private:
	UPROPERTY(Replicated)
	FArenaInventoryList InventoryList;
};
