// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Equipment/ArenaEquipmentInstance.h"
#include "ArenaGrenadeInstance.generated.h"

class UArenaGrenadeDefinitionData;
class AArenaGrenadeBase;
class UGameplayEffect;
class UNiagaraSystem;


/**
 * 
 */
UCLASS()
class ARENA_API UArenaGrenadeInstance : public UArenaEquipmentInstance
{
	GENERATED_BODY()

public:
	UArenaGrenadeInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	UPROPERTY(EditDefaultsOnly)
	UArenaGrenadeDefinitionData* GrenadeDefinitionData;

public:
	UFUNCTION(BlueprintCallable)
	UArenaGrenadeDefinitionData* GetGrenadeDefinitionData() const;
};
