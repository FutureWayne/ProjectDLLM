// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaAttributeSet.h"
#include "ArenaCombatSet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FArenaCombat_AttributeChanged, float, OldValue, float, NewValue, AActor*, Instigator);

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

	UPROPERTY(BlueprintAssignable)
	FArenaCombat_AttributeChanged OnBurstCountChanged;

	UPROPERTY(BlueprintAssignable)
	FArenaCombat_AttributeChanged OnSpeedBuffChanged;

protected:
	UFUNCTION()
	void OnRep_BurstCount(const FGameplayAttributeData& OldBurstCount);

	UFUNCTION()
	void OnRep_SpeedBuff(const FGameplayAttributeData& OldSpeedBuff);

	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BurstCount, Category = "Arena|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BurstCount;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpeedBuff, Category = "Arena|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData SpeedBuff;
};
