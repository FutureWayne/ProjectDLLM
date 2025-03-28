// Copyright Ludens Studio. All Rights Reserved.

#include "System/ArenaSystemStatics.h"

#include "ArenaLogChannel.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/MeshComponent.h"
#include "Equipment/ArenaEquipmentDefinition.h"
#include "Equipment/ArenaEquipmentInstance.h"
#include "Equipment/ArenaEquipmentManagerComponent.h"
#include "Equipment/ArenaQuickBarComponent.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Inventory/ArenaInventoryManagerComponent.h"
#include "Inventory/InventoryFragment_EquippableItem.h"
#include "Inventory/InventoryFragment_LoadoutItemData.h"
#include "Weapon/ArenaGrenadeBase.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaSystemStatics)

void UArenaSystemStatics::PlayNextGame(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		return;
	}

	const FWorldContext& WorldContext = GEngine->GetWorldContextFromWorldChecked(World);
	FURL LastURL = WorldContext.LastURL;

#if WITH_EDITOR
	// To transition during PIE we need to strip the PIE prefix from maps.
	LastURL.Map = UWorld::StripPIEPrefixFromPackageName(LastURL.Map, WorldContext.World()->StreamingLevelsPrefix);
#endif

	// Add seamless travel option as we want to keep clients connected. This will fall back to hard travel if seamless is disabled
	LastURL.AddOption(TEXT("SeamlessTravel"));

	FString URL = LastURL.ToString();
	
	// If we don't remove the host/port info the server travel will fail.
	URL.RemoveFromStart(LastURL.GetHostPortString());

	const bool bAbsolute = false;
	const bool bShouldSkipGameNotify = false;
	World->ServerTravel(URL, bAbsolute, bShouldSkipGameNotify);
}

void UArenaSystemStatics::SetScalarParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const float ParameterValue, bool bIncludeChildActors)
{
	if (TargetActor != nullptr)
	{
		TargetActor->ForEachComponent<UMeshComponent>(bIncludeChildActors, [=](UMeshComponent* InComponent)
		{
			InComponent->SetScalarParameterValueOnMaterials(ParameterName, ParameterValue);
		});
	}
}

void UArenaSystemStatics::SetVectorParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const FVector ParameterValue, bool bIncludeChildActors)
{
	if (TargetActor != nullptr)
	{
		TargetActor->ForEachComponent<UMeshComponent>(bIncludeChildActors, [=](UMeshComponent* InComponent)
		{
			InComponent->SetVectorParameterValueOnMaterials(ParameterName, ParameterValue);
		});
	}
}

void UArenaSystemStatics::SetColorParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const FLinearColor ParameterValue, bool bIncludeChildActors)
{
	SetVectorParameterValueOnAllMeshComponents(TargetActor, ParameterName, FVector(ParameterValue), bIncludeChildActors);
}

TArray<UActorComponent*> UArenaSystemStatics::FindComponentsByClass(AActor* TargetActor, TSubclassOf<UActorComponent> ComponentClass, bool bIncludeChildActors)
{
	TArray<UActorComponent*> Components;
	if (TargetActor != nullptr)
	{
		TargetActor->GetComponents(ComponentClass, /*out*/ Components, bIncludeChildActors);
	}
	return MoveTemp(Components);
}

AArenaGrenadeBase* UArenaSystemStatics::SpawnGrenadeByGrenadeDefinition(const UObject* WorldContextObject, const FTransform& SpawnTransform, const UArenaGrenadeDefinitionData* GrenadeDefinitionData ,
                                                                      AActor* Owner, APawn* Instigator)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr || GrenadeDefinitionData == nullptr)
	{
		return nullptr;
	}
	
	check(GrenadeDefinitionData->GrenadeClass);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Owner;
	SpawnParameters.Instigator = Instigator;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AArenaGrenadeBase* Grenade = World->SpawnActor<AArenaGrenadeBase>(GrenadeDefinitionData->GrenadeClass, SpawnTransform, SpawnParameters);
	if (Grenade)
	{
		Grenade->SetGrenadeParameter(GrenadeDefinitionData);
	}

	return Grenade;
}

UArenaInventoryItemInstance* UArenaSystemStatics::GiveItemDefinitionToPlayer(AController* TargetController,
                                                                             const TSubclassOf<UArenaInventoryItemDefinition> ItemDefinitionClass,
                                                                             const int32 Count, const bool bAutoAddToQuickbar)
{
	if (!TargetController || !TargetController->HasAuthority())
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Invalid Pawn or Pawn is not authoritative"));
		return nullptr;
	}
	
	UArenaInventoryManagerComponent* InventoryManager = TargetController->GetComponentByClass<UArenaInventoryManagerComponent>();
	if (!InventoryManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Inventory Manager found on %s"), *TargetController->GetName());
		return nullptr;
	}

	// Add the item to inventory
	UArenaInventoryItemInstance* RetItemInstance = InventoryManager->AddItemDefinition(ItemDefinitionClass, Count);

	// Add the item to quickbar
	if (bAutoAddToQuickbar)
	{
		EquipItemToQuickBar(TargetController, RetItemInstance);
	}

	return RetItemInstance;
}

void UArenaSystemStatics::EquipItemToQuickBar(AController* TargetController, UArenaInventoryItemInstance* ItemInstance)
{
	if (!TargetController || !TargetController->HasAuthority())
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Invalid Pawn or Pawn is not authoritative"));
		return;
	}
	
	UArenaQuickBarComponent* QuickBar = TargetController->FindComponentByClass<UArenaQuickBarComponent>();
	if (!QuickBar)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No QuickBar found on %s"), *TargetController->GetName());
		return;
	}

	UArenaEquipmentManagerComponent* EquipmentManager = TargetController->FindComponentByClass<UArenaEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Equipment Manager found on %s"), *TargetController->GetName());
		return;
	}
	
	const UInventoryFragment_EquippableItem* EquippableItem = ItemInstance->FindFragmentByClass<UInventoryFragment_EquippableItem>();
	if (!EquippableItem)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Item %s is not equippable"), *ItemInstance->GetName());
		return;
	}

	int32 NextSlotIndex = QuickBar->GetNextFreeItemSlot();
	if (NextSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No free slot found in QuickBar"));
		return;
	}

	QuickBar->AddItemToSlot(NextSlotIndex, ItemInstance);

	int32 ActiveSlot = QuickBar->GetActiveSlotIndex();
	if (ActiveSlot == INDEX_NONE)
	{
		QuickBar->SetActiveSlotIndex(NextSlotIndex);
	}
}

void UArenaSystemStatics::ClearQuickBar(AController* TargetController)
{
	if (!TargetController || !TargetController->HasAuthority())
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Invalid TargetController or TargetController is not authoritative"));
		return;
	}

	UArenaInventoryManagerComponent* InventoryManager = TargetController->GetComponentByClass<UArenaInventoryManagerComponent>();
	if (!InventoryManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Inventory Manager found on %s"), *TargetController->GetName());
		return;
	}
	
	UArenaQuickBarComponent* QuickBar = TargetController->FindComponentByClass<UArenaQuickBarComponent>();
	if (!QuickBar)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No QuickBar found on %s"), *TargetController->GetName());
		return;
	}

	TArray<UArenaInventoryItemInstance*> AllSlots = QuickBar->GetSlots();
	for (int32 index = 0; index < AllSlots.Num(); index++)
	{
		UArenaInventoryItemInstance* ItemInstance = AllSlots[index];
		if (ItemInstance)
		{
			QuickBar->RemoveItemFromSlot(index);
			InventoryManager->RemoveItemInstance(ItemInstance);
		}
	}
}

void UArenaSystemStatics::ClearInventory(AController* TargetController)
{
	if (!TargetController || !TargetController->HasAuthority())
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Invalid TargetController"));
		return;
	}

	UArenaInventoryManagerComponent* InventoryManager = TargetController->GetComponentByClass<UArenaInventoryManagerComponent>();
	if (!InventoryManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Inventory Manager found on %s"), *TargetController->GetName());
		return;
	}

	APawn* TargetPawn = TargetController->GetPawn();
	if (!TargetPawn)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Pawn found on %s"), *TargetController->GetName());
		return;
	}
	
	UArenaEquipmentManagerComponent* EquipmentManager = TargetPawn->FindComponentByClass<UArenaEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Equipment Manager found on %s"), *TargetPawn->GetName());
		return;
	}

	// 1. Clear the quickbar to handle unequip logic
	ClearQuickBar(TargetController);

	// 2. Clear equipment list
	EquipmentManager->UnequipAll();

	// 3. Clear the inventory item that not in quickbar
	InventoryManager->ClearInventory();
}

void UArenaSystemStatics::AddLoadoutToInventory(AController* TargetController,
	const TArray<TSubclassOf<UArenaInventoryItemDefinition>>& LoadoutItemList)
{
	APawn* TargetPawn = TargetController->GetPawn();
	if (!TargetPawn)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Pawn found on %s"), *TargetController->GetName());
		return;
	}
	
	UArenaEquipmentManagerComponent* EquipmentManager = TargetPawn->FindComponentByClass<UArenaEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Equipment Manager found on %s"), *TargetPawn->GetName());
		return;
	}
	
	for (const TSubclassOf<UArenaInventoryItemDefinition>& ItemClass : LoadoutItemList)
	{
		if (ItemClass == nullptr)
		{
			UE_LOG(LogArenaInventory, Warning, TEXT("Invalid item class in loadout"));
			continue;
		}
		
		// Simply add the loadout item to the inventory, handle equip logic later based on the loadout type
		UArenaInventoryItemInstance* LoadoutItemInstance = GiveItemDefinitionToPlayer(TargetController, ItemClass, 1, false);
		if (!LoadoutItemInstance)
		{
			UE_LOG(LogArenaInventory, Warning, TEXT("Failed to add loadout item %s to inventory"), *ItemClass->GetName());
			continue;
		}
		
		const UInventoryFragment_LoadoutItemData* LoadoutItemData = LoadoutItemInstance->FindFragmentByClass<UInventoryFragment_LoadoutItemData>();
		if (!LoadoutItemData)
		{
			UE_LOG(LogArenaInventory, Warning, TEXT("Item %s is not a loadout item"), *LoadoutItemInstance->GetName());
			continue;
		}

		ELoadoutType LoadoutType = LoadoutItemData->ItemLoadoutType;

		// For grenades loadout, add to equip list
		if (LoadoutType > ELoadoutType::MovementAbility)
		{
			if (const UInventoryFragment_EquippableItem* EquipInfo = LoadoutItemInstance->FindFragmentByClass<UInventoryFragment_EquippableItem>())
			{
				TSubclassOf<UArenaEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
				if (EquipDef != nullptr)
				{
					auto EquippedItem = EquipmentManager->EquipItem(EquipDef, LoadoutItemInstance);
				}
			}
		}

		// For ability loadout, InventoryFragment_AddAbilitySet handles adding ability
	}
}
