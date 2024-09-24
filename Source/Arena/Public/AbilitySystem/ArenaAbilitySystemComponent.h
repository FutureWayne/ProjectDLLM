// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ArenaGameplayAbility.h"
#include "ArenaAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UArenaAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UArenaAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	void AddCharacterAbilities(const TArray<TSubclassOf<class UGameplayAbility>>& StartupAbilities);
	
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	typedef TFunctionRef<bool(const UArenaGameplayAbility* ArenaAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	void CancelAbilitiesByFunc(const TShouldCancelAbilityFunc& ShouldCancelFunc, bool bReplicateCancelAbility);

	bool IsActivationGroupBlocked(EArenaAbilityActivationGroup Group) const;
	void AddAbilityToActivationGroup(EArenaAbilityActivationGroup Group, UArenaGameplayAbility* ArenaAbility);
	void RemoveAbilityFromActivationGroup(EArenaAbilityActivationGroup Group, const UArenaGameplayAbility* ArenaAbility);
	void CancelActivationGroupAbilities(EArenaAbilityActivationGroup Group, UArenaGameplayAbility* IgnoreArenaAbility, bool bReplicateCancelAbility);

protected:
	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;
	
protected:
	
	// Handles to abilities that had their input pressed this frame.
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// Handles to abilities that had their input released this frame.
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// Handles to abilities that have their input held.
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
	
	// Number of abilities running in each activation group.
	int32 ActivationGroupCounts[static_cast<uint8>(EArenaAbilityActivationGroup::MAX)];
};
