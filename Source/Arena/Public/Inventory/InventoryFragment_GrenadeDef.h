// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_GrenadeDef.generated.h"

class UNiagaraSystem;
class UGameplayEffect;

USTRUCT(Blueprintable)
struct FGrenadeDefinitionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float TimeBeforeExplosion = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float DetonationRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float ProjectileSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	bool bShouldBounce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UNiagaraSystem> TrailEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	FGameplayTag ExplosionCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TSubclassOf<UGameplayEffect> ExplosionGameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TSubclassOf<UGameplayEffect> DirectHitGameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<USoundBase> GrenadeImpactSound;
};

/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_GrenadeDef : public UArenaInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=GrenadeInfo)
	TSubclassOf<AActor> GrenadeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=GrenadeInfo)
	FGrenadeDefinitionData GrenadeData;
};
