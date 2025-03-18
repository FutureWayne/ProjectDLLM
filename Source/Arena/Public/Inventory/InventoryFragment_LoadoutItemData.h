// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_LoadoutItemData.generated.h"

UENUM(BlueprintType)
enum class ELoadoutType : uint8
{
	Ability,
	FragGrenade,
	EffectGrenade,
	DeploymentGrenade,
};

/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_LoadoutItemData : public UArenaInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	FName ItemDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	FText ItemDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	TObjectPtr<UTexture> IconTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	ELoadoutType ItemLoadoutType;
};
