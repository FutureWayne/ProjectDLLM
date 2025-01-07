// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "AbilitySystem/ArenaAbilitySet.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "ArenaEquipmentManagerComponent.generated.h"


struct FArenaEquipmentList;
class UArenaEquipmentManagerComponent;
class UArenaEquipmentDefinition;
class UArenaEquipmentInstance;

/** A single piece of applied equipment */
USTRUCT(BlueprintType)
struct FArenaAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FArenaAppliedEquipmentEntry()
	{}

	FString GetDebugString() const;

private:
	friend FArenaEquipmentList;
	friend UArenaEquipmentManagerComponent;

	UPROPERTY()
	TSubclassOf<UArenaEquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	TObjectPtr<UArenaEquipmentInstance> Instance = nullptr;

	// Authority-only list of granted handles
	UPROPERTY(NotReplicated)
	FArenaAbilitySet_GrantedHandles GrantedHandles;
};

/** List of applied equipment */
USTRUCT(BlueprintType)
struct FArenaEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FArenaEquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FArenaEquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FArenaAppliedEquipmentEntry, FArenaEquipmentList>(Entries, DeltaParms, *this);
	}

	UArenaEquipmentInstance* AddEntry(const TSubclassOf<UArenaEquipmentDefinition>& EquipmentDefinition);
	void RemoveEntry(UArenaEquipmentInstance* EquipmentInstance);

private:
	UArenaAbilitySystemComponent* GetAbilitySystemComponent() const;

	friend UArenaEquipmentManagerComponent;

private:
	UPROPERTY()
	TArray<FArenaAppliedEquipmentEntry> Entries;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FArenaEquipmentList> : public TStructOpsTypeTraitsBase2<FArenaEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * Manages equipment applied to a pawn
 */
UCLASS(BlueprintType, Const)
class ARENA_API UArenaEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()
	
public:
    UArenaEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Equipment)
    UArenaEquipmentInstance* EquipItem(TSubclassOf<UArenaEquipmentDefinition> EquipmentClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Equipment)
	void UnequipItem(UArenaEquipmentInstance* ItemInstance);

	// ~UObject interface
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	// ~End of UObject interface

	// ~UActorComponent interface
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void ReadyForReplication() override;
	// ~End of UActorComponent interface

	/** Returns the first equipped instance of a given type, or nullptr if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Equipment)
	UArenaEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UArenaEquipmentInstance> InstanceType);

	/** Returns all equipped instances of a given type, or an empty array if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Equipment)
	TArray<UArenaEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UArenaEquipmentInstance> InstanceType) const;

	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return static_cast<T*>(GetFirstInstanceOfType(T::StaticClass()));
	}
	
private:
	UPROPERTY(Replicated)
	FArenaEquipmentList EquipmentList;
};
