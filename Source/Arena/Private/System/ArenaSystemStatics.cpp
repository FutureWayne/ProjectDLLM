// Copyright Ludens Studio. All Rights Reserved.

#include "System/ArenaSystemStatics.h"

#include "ArenaLogChannel.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/MeshComponent.h"
#include "Equipment/ArenaQuickBarComponent.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Inventory/ArenaInventoryManagerComponent.h"
#include "Inventory/InventoryFragment_EquippableItem.h"
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

AArenaGrenadeBase* UArenaSystemStatics::SpawnGrenadeByGrenadeInstance(const UObject* WorldContextObject, const FTransform& SpawnTransform, UArenaGrenadeDefinitionData* GrenadeDefinitionData ,
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

UArenaInventoryItemInstance* UArenaSystemStatics::GiveItemDefinitionToPlayer(APawn* ReceivingPawn,
                                                                             const TSubclassOf<UArenaInventoryItemDefinition> ItemDefinitionClass,
                                                                             const int32 Count, const bool bAutoEquip)
{
	if (!ReceivingPawn || !ReceivingPawn->HasAuthority())
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Invalid Pawn or Pawn is not authoritative"));
		return nullptr;
	}
	
	AController* Controller = ReceivingPawn->GetController();
	if (!Controller)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Controller found on %s"), *ReceivingPawn->GetName());
		return nullptr;
	}
	
	UArenaInventoryManagerComponent* InventoryManager = Controller->GetComponentByClass<UArenaInventoryManagerComponent>();
	if (!InventoryManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Inventory Manager found on %s"), *ReceivingPawn->GetName());
		return nullptr;
	}

	// Add the item to inventory
	UArenaInventoryItemInstance* RetItemInstance = InventoryManager->AddItemDefinition(ItemDefinitionClass, Count);

	// Add the item to quickbar
	if (bAutoEquip)
	{
		EquipItemToQuickBar(ReceivingPawn, RetItemInstance);
	}

	return RetItemInstance;
}

void UArenaSystemStatics::EquipItemToQuickBar(const APawn* ReceivingPawn, UArenaInventoryItemInstance* ItemInstance)
{
	if (!ReceivingPawn || !ReceivingPawn->HasAuthority())
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Invalid Pawn or Pawn is not authoritative"));
		return;
	}
	
	AController* Controller = ReceivingPawn->GetController();
	if (!Controller)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Controller found on %s"), *ReceivingPawn->GetName());
		return;
	}
	
	UArenaQuickBarComponent* QuickBar = Controller->FindComponentByClass<UArenaQuickBarComponent>();
	if (!QuickBar)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No QuickBar found on %s"), *ReceivingPawn->GetName());
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

void UArenaSystemStatics::DropAllEquippedItemInQuickBar(const APawn* DroppingPawn)
{
	if (!DroppingPawn || !DroppingPawn->HasAuthority())
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("Invalid Pawn or Pawn is not authoritative"));
		return;
	}
	
	AController* Controller = DroppingPawn->GetController();
	if (!Controller)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Controller found on %s"), *DroppingPawn->GetName());
		return;
	}

	UArenaInventoryManagerComponent* InventoryManager = Controller->GetComponentByClass<UArenaInventoryManagerComponent>();
	if (!InventoryManager)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No Inventory Manager found on %s"), *DroppingPawn->GetName());
		return;
	}
	
	UArenaQuickBarComponent* QuickBar = Controller->FindComponentByClass<UArenaQuickBarComponent>();
	if (!QuickBar)
	{
		UE_LOG(LogArenaInventory, Warning, TEXT("No QuickBar found on %s"), *DroppingPawn->GetName());
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

