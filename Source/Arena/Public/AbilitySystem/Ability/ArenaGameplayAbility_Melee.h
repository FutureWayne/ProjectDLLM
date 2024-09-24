// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ArenaGameplayAbility.h"
#include "ArenaGameplayAbility_Melee.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class ARENA_API UArenaGameplayAbility_Melee : public UArenaGameplayAbility
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
