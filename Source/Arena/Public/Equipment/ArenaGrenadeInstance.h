// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/ArenaEquipmentInstance.h"
#include "ArenaGrenadeInstance.generated.h"

USTRUCT(Blueprintable)
struct FGrenadeParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Grenade Params")
	float TimeToExplode;
	
};

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Grenade Params")
	FGrenadeParams GrenadeParams;
};
