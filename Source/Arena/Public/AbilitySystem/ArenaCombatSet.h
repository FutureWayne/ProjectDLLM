// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaAttributeSet.h"
#include "ArenaCombatSet.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UArenaCombatSet : public UArenaAttributeSet
{
	GENERATED_BODY()

public:
	UArenaCombatSet();

	ATTRIBUTE_ACCESSORS(UArenaCombatSet, BurstCount);
	ATTRIBUTE_ACCESSORS(UArenaCombatSet, SpeedBuff);

	mutable FArenaAttributeEvent OnBurstCountChanged;
	mutable FArenaAttributeEvent OnSpeedBuffChanged;

protected:
	UFUNCTION()
	void OnRep_BurstCount(const FGameplayAttributeData& OldBurstCount);

	UFUNCTION()
	void OnRep_SpeedBuff(const FGameplayAttributeData& OldSpeedBuff);

	
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BurstCount, Category = "Arena|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BurstCount;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpeedBuff, Category = "Arena|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData SpeedBuff;
};
