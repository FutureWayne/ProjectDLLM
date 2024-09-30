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

	/**
	 * Sync Time between Server and Client
	 */

	// Requests the current server time, passing the client's request time
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports current server time, passing the client's request time and the server's response time
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfServerReceivedRequest, float TimeOfClientRequest);

	UFUNCTION(BlueprintCallable, Category = "Arena|Time Sync")
	float GetServerTime();

	// ~APlayerController interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void ReceivedPlayer() override;
	// ~APlayerController interface

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

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

	/*
	 * Server - Client Time Sync
	 */
	
	// Time between the client and server
	float ClientServerDeltaTime = 0.f;
	
	float TimeSyncRunningTime = 0.f;
	
	UPROPERTY(EditAnywhere, Category = "Arena|Time Sync")
	float TimeSyncFrequency = 5.f;

private:
	void CheckTimeSync(float DeltaTime);
};
