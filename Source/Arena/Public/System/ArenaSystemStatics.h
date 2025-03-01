// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/SoftObjectPtr.h"
#include "Weapon/ArenaGrenadeBase.h"

#include "ArenaSystemStatics.generated.h"

class UArenaGrenadeDefinitionData;
class UArenaInventoryItemDefinition;
class UArenaInventoryItemInstance;
template <typename T> class TSubclassOf;

class AActor;
class UActorComponent;
class UObject;
struct FFrame;

UCLASS()
class ARENA_API UArenaSystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Arena", meta = (WorldContext = "WorldContextObject"))
	static void PlayNextGame(const UObject* WorldContextObject);

	// Sets ParameterName to ParameterValue on all sections of all mesh components found on the TargetActor
	UFUNCTION(BlueprintCallable, Category = "Rendering|Material", meta=(DefaultToSelf="TargetActor"))
	static void SetScalarParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const float ParameterValue, bool bIncludeChildActors = true);

	// Sets ParameterName to ParameterValue on all sections of all mesh components found on the TargetActor
	UFUNCTION(BlueprintCallable, Category = "Rendering|Material", meta=(DefaultToSelf="TargetActor"))
	static void SetVectorParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const FVector ParameterValue, bool bIncludeChildActors = true);

	// Sets ParameterName to ParameterValue on all sections of all mesh components found on the TargetActor
	UFUNCTION(BlueprintCallable, Category = "Rendering|Material", meta=(DefaultToSelf="TargetActor"))
	static void SetColorParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const FLinearColor ParameterValue, bool bIncludeChildActors = true);

	// Gets all the components that inherit from the given class
	UFUNCTION(BlueprintCallable, Category = "Actor", meta=(DefaultToSelf="TargetActor", ComponentClass="/Script/Engine.ActorComponent", DeterminesOutputType="ComponentClass"))
	static TArray<UActorComponent*> FindComponentsByClass(AActor* TargetActor, TSubclassOf<UActorComponent> ComponentClass, bool bIncludeChildActors = true);

	UFUNCTION(BlueprintCallable, Category = "Arena|Grenade", meta = (WorldContext = "WorldContextObject"))
	static AArenaGrenadeBase* SpawnGrenadeByGrenadeDefinition(const UObject* WorldContextObject, const FTransform& SpawnTransform,
	                                                          const UArenaGrenadeDefinitionData* GrenadeDefinitionData, AActor* Owner = nullptr,
	                                                          APawn* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Arena|Inventory")
	static UArenaInventoryItemInstance* GiveItemDefinitionToPlayer(APawn* ReceivingPawn, TSubclassOf<UArenaInventoryItemDefinition> ItemDefinitionClass,
																	int32 Count = 1, bool bAutoEquip = false);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Arena|Inventory")
	static void EquipItemToQuickBar(const APawn* ReceivingPawn, UArenaInventoryItemInstance* ItemInstance);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Arena|Inventory")
	static void DropAllEquippedItemInQuickBar(const APawn* DroppingPawn);
};