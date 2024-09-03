// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaAttributeSet.h"
#include "ArenaHealthSet.generated.h"

struct FGameplayEffectModCallbackData;

/**
 * 
 */
UCLASS()
class ARENA_API UArenaHealthSet : public UArenaAttributeSet
{
	GENERATED_BODY()

public:
	UArenaHealthSet();

	ATTRIBUTE_ACCESSORS(UArenaHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UArenaHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UArenaHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(UArenaHealthSet, Damage);

	mutable FArenaAttributeEvent OnHealthChanged;
	mutable FArenaAttributeEvent OnMaxHealthChanged;
	mutable FArenaAttributeEvent OnOutOfHealth;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// The current health attribute.  The health will be capped by the max health attribute.  Health is hidden from modifiers so only executions can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Arena|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	// The current max health attribute.  Max health is an attribute since gameplay effects can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Arena|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;
	
	// Used to track when the health reaches 0.
	bool bOutOfHealth;

	// Store the health before any changes 
	float MaxHealthBeforeAttributeChange;
	float HealthBeforeAttributeChange;

	// -------------------------------------------------------------------
	//	Meta Attribute (please keep attributes that aren't 'stateful' below 
	// -------------------------------------------------------------------

	// Incoming healing. This is mapped directly to +Health
	UPROPERTY(BlueprintReadOnly, Category="Arena|Health", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Healing;

	// Incoming damage. This is mapped directly to -Health
	UPROPERTY(BlueprintReadOnly, Category="Arena|Health", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Damage;
};
