// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/ArenaGameplayAbility_FromGrenade.h"
#include "ArenaGameplayAbility_GrenadeFire.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UArenaGameplayAbility_GrenadeFire : public UArenaGameplayAbility_FromGrenade
{
	GENERATED_BODY()
	
public:

	UArenaGameplayAbility_GrenadeFire(const FObjectInitializer& ObjectInitializer);
	
	// ~Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual bool CommitAbilityCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const bool ForceCooldown, FGameplayTagContainer* OptionalRelevantTags) override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	// ~End UGameplayAbility Interface

protected:
	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Spawn Grenade"))
	AArenaGrenadeBase* SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation);

	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "BroadCast Cooldown Message"))
	void BroadCastCooldownMessage();

private:
	FActiveGameplayEffectHandle CooldownEffectHandle;
};
