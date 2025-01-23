// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/GameState.h"
#include "ArenaGameState.generated.h"

class UArenaAbilitySystemComponent;
class AArenaPlayerState;
/**
 * 
 */
UCLASS()
class ARENA_API AArenaGameState : public AGameState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AArenaGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface
	
	UFUNCTION(BlueprintCallable, Category = "Arena|GameState")
	UArenaAbilitySystemComponent* GetArenaAbilitySystemComponent() const { return AbilitySystemComponent; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Arena|GameState")
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystemComponent;
	
public:
	TArray<AArenaPlayerState*> AttackTeam;
	TArray<AArenaPlayerState*> DefenseTeam;
};
