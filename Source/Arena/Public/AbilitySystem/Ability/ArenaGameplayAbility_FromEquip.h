// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "AbilitySystem/ArenaGameplayAbility.h"
#include "ArenaGameplayAbility_FromEquip.generated.h"

class UArenaInventoryItemInstance;
class UArenaEquipmentInstance;

/**
 * 
 */
UCLASS()
class ARENA_API UArenaGameplayAbility_FromEquip : public UArenaGameplayAbility
{
	GENERATED_BODY()

public:
	UArenaGameplayAbility_FromEquip(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, Category="Arena|Ability")
	UArenaEquipmentInstance* GetAssociatedEquipment() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|Ability")
	UArenaInventoryItemInstance* GetAssociatedItem() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

};
