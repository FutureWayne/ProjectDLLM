// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ArenaGameplayAbility.h"
#include "ArenaMeleeAbility.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UArenaMeleeAbility : public UArenaGameplayAbility
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Melee")
	void TraceMeleeTarget();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float MeleeRange = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	TSubclassOf<UGameplayEffect> MeleeDamageEffectClass;
	
};
