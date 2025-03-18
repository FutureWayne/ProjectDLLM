// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaLoadoutTypeData.generated.h"

enum class ELoadoutType : uint8;

/**
 * 
 */
UCLASS(BlueprintType)
class ARENA_API UArenaLoadoutTypeData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	FName LoadoutName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	FText LoadoutDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	FText LoadoutDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	ELoadoutType LoadoutType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Loadout")
	TArray<TSubclassOf<class UArenaInventoryItemDefinition>> InventoryIDList;
};
