// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ArenaCharacterBase.generated.h"

class UGameplayAbility;
class UInputMappingContext;
class UArenaAbilitySystemComponent;
class UArenaHealthComponent;
class UArenaHealthSet;
class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class ARENA_API AArenaCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AArenaCharacterBase();

	UFUNCTION(BlueprintCallable, Category = "Arena|Character")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UArenaHealthSet* GetArenaHealthSet() const;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void AddCharacterAbilities() const;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UArenaHealthSet> ArenaHealthSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaHealthComponent> HealthComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Abilities", Meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
};
