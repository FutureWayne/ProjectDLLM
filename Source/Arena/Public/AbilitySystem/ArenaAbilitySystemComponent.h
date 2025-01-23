// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ArenaGameplayAbility.h"
#include "NativeGameplayTags.h"
#include "ArenaAbilitySystemComponent.generated.h"

ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

/**
 * 
 */
UCLASS()
class ARENA_API UArenaAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UArenaAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();

	typedef TFunctionRef<bool(const UArenaGameplayAbility* ArenaAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	void CancelAbilitiesByFunc(const TShouldCancelAbilityFunc& ShouldCancelFunc, bool bReplicateCancelAbility);

	bool IsActivationGroupBlocked(EArenaAbilityActivationGroup Group) const;
	void AddAbilityToActivationGroup(EArenaAbilityActivationGroup Group, UArenaGameplayAbility* ArenaAbility);
	void RemoveAbilityFromActivationGroup(EArenaAbilityActivationGroup Group, const UArenaGameplayAbility* ArenaAbility);
	void CancelActivationGroupAbilities(EArenaAbilityActivationGroup Group, UArenaGameplayAbility* IgnoreArenaAbility, bool bReplicateCancelAbility);

protected:
	void TryActivateAbilitiesOnSpawn();
	
	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;

	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
	
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
