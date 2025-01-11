// Copyright Ludens Studio. All Rights Reserved.


#include "Equipment/ArenaEquipmentManagerComponent.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Engine/ActorChannel.h"
#include "Equipment/ArenaEquipmentDefinition.h"
#include "Equipment/ArenaEquipmentInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaEquipmentManagerComponent)


//////////////////////////////////////////////////////////////////////
// FArenaAppliedEquipmentEntry

FString FArenaAppliedEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s of %s"), *GetNameSafe(Instance), *GetNameSafe(EquipmentDefinition.Get()));
}

//////////////////////////////////////////////////////////////////////
// FArenaEquipmentList

void FArenaEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FArenaAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnUnequipped();
		}
	}
}

void FArenaEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FArenaAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnEquipped();
		}
	}
}

void FArenaEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
}

UArenaEquipmentInstance* FArenaEquipmentList::AddEntry(const TSubclassOf<UArenaEquipmentDefinition>& EquipmentDefinition)
{
	UArenaEquipmentInstance* Result = nullptr;
	UArenaAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	
	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());
	check(ASC);
	
	const UArenaEquipmentDefinition* EquipmentCDO = GetDefault<UArenaEquipmentDefinition>(EquipmentDefinition);

	TSubclassOf<UArenaEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = UArenaEquipmentInstance::StaticClass();
	}

	FArenaAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	NewEntry.Instance = NewObject<UArenaEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	Result = NewEntry.Instance;
	
	for (const TObjectPtr<const UArenaAbilitySet>& AbilitySet : EquipmentCDO->AbilitySetsToGrant)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(ASC, /*inout*/ &NewEntry.GrantedHandles, Result);
		}
	}

	Result->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn);

	MarkItemDirty(NewEntry);

	return Result;
}

void FArenaEquipmentList::RemoveEntry(UArenaEquipmentInstance* EquipmentInstance)
{
	UArenaAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	check(ASC);
	
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FArenaAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == EquipmentInstance)
		{
			Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
		}

		EquipmentInstance->DestroyEquipmentActors();

		EntryIt.RemoveCurrent();
		MarkArrayDirty();
	}
}

UArenaAbilitySystemComponent* FArenaEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<UArenaAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

//////////////////////////////////////////////////////////////////////
// ULyraEquipmentManagerComponent

UArenaEquipmentManagerComponent::UArenaEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UArenaEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquipmentList);
}

UArenaEquipmentInstance* UArenaEquipmentManagerComponent::EquipItem(
	const TSubclassOf<UArenaEquipmentDefinition> EquipmentClass)
{
	UArenaEquipmentInstance* Result = nullptr;

	if (EquipmentClass != nullptr)
	{
		Result = EquipmentList.AddEntry(EquipmentClass);
		if (Result != nullptr)
		{
			Result->OnEquipped();

			if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
			{
				AddReplicatedSubObject(Result);
			}
		}
	}
	return Result;
}

void UArenaEquipmentManagerComponent::UnequipItem(UArenaEquipmentInstance* ItemInstance)
{
	if (ItemInstance != nullptr)
	{
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(ItemInstance);
		}

		ItemInstance->OnUnequipped();
		EquipmentList.RemoveEntry(ItemInstance);
	}
}

bool UArenaEquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FArenaAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance != nullptr)
		{
			WroteSomething |= Channel->ReplicateSubobject(Entry.Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UArenaEquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UArenaEquipmentManagerComponent::UninitializeComponent()
{
	TArray<UArenaEquipmentInstance*> AllEquipmentInstances;

	// gathering all instances before removal to avoid side effects affecting the equipment list iterator	
	for (const FArenaAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		AllEquipmentInstances.Add(Entry.Instance);
	}

	for (UArenaEquipmentInstance* EquipInstance : AllEquipmentInstances)
	{
		if (EquipInstance)
		{
			UnequipItem(EquipInstance);
		}
	}
	
	Super::UninitializeComponent();
}

void UArenaEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing equipment instances for replication
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FArenaAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			UArenaEquipmentInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

UArenaEquipmentInstance* UArenaEquipmentManagerComponent::GetFirstInstanceOfType(
	TSubclassOf<UArenaEquipmentInstance> InstanceType)
{
	for (FArenaAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance->IsA(InstanceType))
		{
			return Entry.Instance;
		}
	}

	return nullptr;
}

TArray<UArenaEquipmentInstance*> UArenaEquipmentManagerComponent::GetEquipmentInstancesOfType(
	TSubclassOf<UArenaEquipmentInstance> InstanceType) const
{
	TArray<UArenaEquipmentInstance*> Result;
	for (const FArenaAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance->IsA(InstanceType))
		{
			Result.Add(Entry.Instance);
		}
	}

	return Result;
}
