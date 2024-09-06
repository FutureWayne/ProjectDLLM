// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "ArenaPlayerController.generated.h"

class UArenaInputConfig;
class UInputMappingContext;
class UArenaAbilitySystemComponent;
class AArenaPlayerState;
/**
 * 
 */
UCLASS()
class ARENA_API AArenaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);
	UArenaInputConfig* GetInputConfig() { return InputConfig; };

	AArenaPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Arena|PlayerController")
	AArenaPlayerState* GetArenaPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|PlayerController")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const;

	// ~APlayerController interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	// ~APlayerController interface

protected:
	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;

	virtual void SetupInputComponent() override;

private:
	UPROPERTY()
	TObjectPtr<UArenaAbilitySystemComponent> ArenaAbilitySystemComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaInputConfig> InputConfig;
};
