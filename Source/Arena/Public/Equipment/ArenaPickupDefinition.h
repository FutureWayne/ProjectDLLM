// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaPickupDefinition.generated.h"

class UArenaInventoryItemDefinition;
class UNiagaraSystem;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Const, Meta = (DisplayName = "Arena Pickup Data", ShortTooltip = "Data asset used to configure a pickup."))
class ARENA_API UArenaPickupDefinition : public UDataAsset
{
	GENERATED_BODY()


public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup|Equipment")
	TSubclassOf<UArenaInventoryItemDefinition> InventoryItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup")
	int32 SpawnCoolDownSeconds;

	//Sound to play when picked up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup")
	TObjectPtr<USoundBase> PickedUpSound;

	//Sound to play when pickup is respawned
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup")
	TObjectPtr<USoundBase> RespawnedSound;

	//Particle FX to play when picked up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup")
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	//Particle FX to play when pickup is respawned
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup")
	TObjectPtr<UNiagaraSystem> RespawnedEffect;
};


UCLASS(Blueprintable, BlueprintType, Const, Meta = (DisplayName = "Arena Weapon Pickup Data", ShortTooltip = "Data asset used to configure a weapon pickup."))
class ARENA_API UArenaWeaponPickupDefinition : public UArenaPickupDefinition
{
	GENERATED_BODY()

public:

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup|Mesh")
	FVector WeaponMeshOffset;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Pickup|Mesh")
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);
};

