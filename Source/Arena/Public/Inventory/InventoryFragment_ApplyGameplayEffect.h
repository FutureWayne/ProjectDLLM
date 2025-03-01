// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_ApplyGameplayEffect.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class ARENA_API UInventoryFragment_ApplyGameplayEffect : public UArenaInventoryItemFragment
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UArenaInventoryItemInstance* Instance, AActor* OwnerActor) override;
	virtual void OnInstanceRemoved(UArenaInventoryItemInstance* Instance, AActor* OwnerActor) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effect")
	TArray<TSubclassOf<UGameplayEffect>> GameplayEffectsToApply;

private:
	TArray<FActiveGameplayEffectHandle> ActiveGameplayEffectHandles;

};
