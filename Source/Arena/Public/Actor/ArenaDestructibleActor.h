// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "ArenaDestructibleActor.generated.h"

struct FGameplayEffectSpec;
class UArenaHealthSet;
class UArenaAbilitySystemComponent;

UCLASS()
class ARENA_API AArenaDestructibleActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	AArenaDestructibleActor();

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UArenaHealthSet* GetArenaHealthSet() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Character", meta = (DisplayName = "On Out Of Health"))
	void K2_OnOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser);
	
protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;

};
