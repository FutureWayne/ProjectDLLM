// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArenaHealthComponent.generated.h"

struct FGameplayEffectSpec;
class UArenaHealthComponent;
class UArenaAbilitySystemComponent;
class UArenaHealthSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FArenaHealth_DeathEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FArenaHealth_AttributeChanged, UArenaHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

/**
 * EArenaDeathState
 *
 *	Defines current state of death.
 */
UENUM(BlueprintType)
enum class EArenaDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UArenaHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UArenaHealthComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "Arena|Health")
	static UArenaHealthComponent* FindHealthComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UArenaHealthComponent>() : nullptr); }

	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	void InitializeWithAbilitySystem(UArenaAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	void UninitializeFromAbilitySystem();

	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	EArenaDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	bool IsDeadOrDying() const { return (DeathState > EArenaDeathState::NotDead); }

	virtual void StartDeath();

	virtual void FinishDeath();

	virtual void DamageSelfDestruct(bool bFellOutOfWorld = false);

	UPROPERTY(BlueprintAssignable)
	FArenaHealth_AttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FArenaHealth_AttributeChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FArenaHealth_DeathEvent OnDeathStarted;

	UPROPERTY(BlueprintAssignable)
	FArenaHealth_DeathEvent OnDeathFinished;

protected:

	virtual void OnUnregister() override;

	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	UFUNCTION()
	virtual void OnRep_DeathState(EArenaDeathState OldDeathState);
	
	UPROPERTY()
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UArenaHealthSet> HealthSet;

	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EArenaDeathState DeathState;
	
};
