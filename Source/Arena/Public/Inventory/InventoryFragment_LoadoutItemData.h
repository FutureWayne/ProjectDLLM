// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_LoadoutItemData.generated.h"

ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_LoadoutType_Grenade_PrimaryGrenade);
ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_LoadoutType_Grenade_SecondaryGrenade);
ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_LoadoutType_Ability_DeploymentAbility);
ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_LoadoutType_Ability_MovementAbility);

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
	FGameplayTag LoadoutTypeTag;
};
