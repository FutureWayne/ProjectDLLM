// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "ArenaGlobalAbilitySystem.generated.h"

class UGameplayEffect;
class UGameplayAbility;
class UArenaAbilitySystemComponent;

USTRUCT()
struct FGlobalAppliedAbilityList
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<TObjectPtr<UArenaAbilitySystemComponent>, FGameplayAbilitySpecHandle> Handles;

	void AddToASC(const TSubclassOf<UGameplayAbility>& Ability, UArenaAbilitySystemComponent* ASC);
	void RemoveFromASC(UArenaAbilitySystemComponent* ASC);
	void RemoveFromAll();
};

USTRUCT()
struct FGlobalAppliedEffectList
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<TObjectPtr<UArenaAbilitySystemComponent>, FActiveGameplayEffectHandle> Handles;

	void AddToASC(TSubclassOf<UGameplayEffect> Effect, UArenaAbilitySystemComponent* ASC);
	void RemoveFromASC(UArenaAbilitySystemComponent* ASC);
	void RemoveFromAll();
};


/**
 * 
 */
UCLASS()
class ARENA_API UArenaGlobalAbilitySystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UArenaGlobalAbilitySystem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Arena")
	void ApplyAbilityToAll(TSubclassOf<UGameplayAbility> Ability);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Arena")
	void ApplyEffectToAll(TSubclassOf<UGameplayEffect> Effect);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Arena")
	void RemoveAbilityFromAll(TSubclassOf<UGameplayAbility> Ability);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Arena")
	void RemoveEffectFromAll(TSubclassOf<UGameplayEffect> Effect);

	/** Register an ASC with global system and apply any active global effects/abilities. */
	void RegisterASC(UArenaAbilitySystemComponent* ASC);

	/** Removes an ASC from the global system, along with any active global effects/abilities. */
	void UnregisterASC(UArenaAbilitySystemComponent* ASC);

private:
	UPROPERTY()
	TMap<TSubclassOf<UGameplayAbility>, FGlobalAppliedAbilityList> AppliedAbilities;

	UPROPERTY()
	TMap<TSubclassOf<UGameplayEffect>, FGlobalAppliedEffectList> AppliedEffects;

	UPROPERTY()
	TArray<TObjectPtr<UArenaAbilitySystemComponent>> RegisteredASCs;
};
