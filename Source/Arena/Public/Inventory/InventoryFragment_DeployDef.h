// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ArenaInventoryItemDefinition.h"
#include "InventoryFragment_DeployDef.generated.h"

class UArenaDeploymentDefinitionData;

UCLASS()
class ARENA_API UInventoryFragment_DeployDef : public UArenaInventoryItemFragment
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditDefaultsOnly)
	UArenaDeploymentDefinitionData* DeployDefinitionData;

public:
	UFUNCTION(BlueprintCallable)
	UArenaDeploymentDefinitionData* GetDeployDefinitionData() const;
};
