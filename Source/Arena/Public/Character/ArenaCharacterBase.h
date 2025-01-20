// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ArenaCharacterBase.generated.h"

class UArenaCombatSet;
class UCameraComponent;
class UArenaAbilitySet;
class UGameplayAbility;
class UInputMappingContext;
class UArenaAbilitySystemComponent;
class UArenaHealthComponent;
class UArenaHealthSet;
class UAttributeSet;
class UAbilitySystemComponent;

UCLASS()
class ARENA_API AArenaCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AArenaCharacterBase();

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	UArenaHealthSet* GetArenaHealthSet() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	UArenaCombatSet* GetArenaCombatSet() const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;

	UPROPERTY()
	TObjectPtr<UArenaCombatSet> ArenaCombatSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaAbilitySet> AbilitySet;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
};
