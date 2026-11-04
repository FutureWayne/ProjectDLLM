// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "GameFramework/GameMode.h"
#include "Player/ArenaPlayerController.h"
#include "AbilitySystemInterface.h"
#include "ArenaKillCameraTestPawn.generated.h"

class UArenaAbilitySystemComponent;
class UArenaHealthSet;
class UArenaHealthComponent;

/** Minimal damage recipient exercising the production health and owner RPC path. */
UCLASS(Transient, NotBlueprintable)
class AArenaKillCameraTestPawn : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AArenaKillCameraTestPawn();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	FVector TestCameraOffset = FVector(-300.0, 0.0, 80.0);
	float TestCameraFOV = 80.0f;

	UPROPERTY()
	TObjectPtr<UArenaAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UArenaHealthSet> Health;

	UPROPERTY()
	TObjectPtr<UArenaHealthComponent> HealthComponent;
};

/** Isolated PIE fixture using production controllers and their camera Blueprint. */
UCLASS(Transient)
class AArenaKillCameraTestGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AArenaKillCameraTestGameMode();
};

UCLASS(Transient, NotBlueprintable)
class AArenaKillCameraTestController : public AArenaPlayerController
{
	GENERATED_BODY()

public:
	AArenaKillCameraTestController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
