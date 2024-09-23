// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ArenaGameplayAbility.h"
#include "ArenaGameplayAbility_Death.generated.h"

/**
 *  UArenaGameplayAbility_Death
 *
 *	Gameplay ability used for handling death.
 *	Ability is activated automatically via the "GameplayEvent.Death" ability trigger tag.
 */
UCLASS(Abstract)
class ARENA_API UArenaGameplayAbility_Death : public UArenaGameplayAbility
{
	GENERATED_BODY()
	
public:
	
	UArenaGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// Start Death Sequence
	UFUNCTION(BlueprintCallable, Category = "Arena|Ability")
	void StartDeath();

	// Finish Death Sequence
	UFUNCTION(BlueprintCallable, Category = "Arena|Ability")
	void FinishDeath();

protected:
	// If enabled, the ability will automatically call StartDeath.  FinishDeath is always called when the ability ends if the death was started.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Death")
	bool bAutoStartDeath;
};
