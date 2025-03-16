// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ArenaAbilitySet.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_AddAbilitySet.generated.h"


/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_AddAbilitySet : public UArenaInventoryItemFragment
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UArenaInventoryItemInstance* Instance, AActor* OwnerActor) override;
	virtual void OnInstanceRemoved(UArenaInventoryItemInstance* Instance, AActor* OwnerActor) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Set")
	TObjectPtr<UArenaAbilitySet> AbilitySetToApply;
};
