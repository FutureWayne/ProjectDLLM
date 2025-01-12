// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "ArenaLootBox.generated.h"

struct FGameplayEffectSpec;
class UBoxComponent;
class AArenaWeaponSpawner;
class UArenaLootBoxData;
class UArenaAbilitySystemComponent;
class UArenaHealthSet;

UCLASS()
class ARENA_API AArenaLootBox : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AArenaLootBox();
	
	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UArenaHealthSet* GetArenaHealthSet() const;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	
protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|LootBox")
	TObjectPtr<UArenaLootBoxData> LootBoxData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|LootBox")
	TSubclassOf<AArenaWeaponSpawner> WeaponSpawnerClass;

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
