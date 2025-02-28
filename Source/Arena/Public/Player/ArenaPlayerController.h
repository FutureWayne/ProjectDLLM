// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "CommonPlayerController.h"
#include "Teams/ArenaTeamAgentInterface.h"
#include "ArenaPlayerController.generated.h"

struct FInputActionValue;
class UArenaQuickBarComponent;
class UArenaInventoryManagerComponent;
class AArenaHUD;
class UArenaInputConfig;
class UInputMappingContext;
class UArenaAbilitySystemComponent;
class AArenaPlayerState;

/**
 * 
 */
UCLASS()
class ARENA_API AArenaPlayerController : public ACommonPlayerController, public IArenaTeamAgentInterface
{
	GENERATED_BODY()

public:
	AArenaPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/*
	 * Ability Input
	 */ 
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	void Input_Crouch(const FInputActionValue& InputActionValue);
	void Input_Jump(const FInputActionValue& InputActionValue);
	void Input_Sprint(const FInputActionValue& InputActionValue);
	void Input_Walk(const FInputActionValue& InputActionValue);

	UFUNCTION(BlueprintCallable, Category = "Arena|PlayerController")
	AArenaPlayerState* GetArenaPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "Arena|PlayerController")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const;

	//~ Begin IArenaTeamAgentInterface Interface
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnArenaTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~ End IArenaTeamAgentInterface Interface

protected:
	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface
	
	// ~APlayerController interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// ~APlayerController interface

	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

private:
	UPROPERTY()
	TObjectPtr<UArenaAbilitySystemComponent> ArenaAbilitySystemComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaInputConfig> InputConfig;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaInventoryManagerComponent> InventoryManagerComponent;

	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaQuickBarComponent> QuickBarComponent;

	UPROPERTY()
	FOnArenaTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

private:
	void BroadcastOnPlayerStateChanged();

public:
	FORCEINLINE UArenaInputConfig* GetInputConfig() { return InputConfig; };
};
