// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaLootBoxData.generated.h"

class UArenaWeaponPickupDefinition;

USTRUCT(BlueprintType)
struct FLootSpawningData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UArenaWeaponPickupDefinition> PickupDefinition;

	UPROPERTY(EditDefaultsOnly)
	float SpawnChance;
};

/**
 * 
 */
UCLASS()
class ARENA_API UArenaLootBoxData : public UDataAsset
{
	GENERATED_BODY()

public:
	UArenaWeaponPickupDefinition* GetRandomPickupDefinition() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|LootBox")
	TArray<FLootSpawningData> LootSpawningData;

};
