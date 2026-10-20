// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Teams/ArenaTeamAgentInterface.h"
#include "GameplayEffect.h"
#include "ArenaIncendiaryTestPawn.generated.h"

class UAbilitySystemComponent;
class UArenaHealthSet;
class UArenaCombatSet;

/** Transient editor-only fixture with the same capsule, GAS and team interfaces as a game character. */
UCLASS(Transient, NotBlueprintable)
class AArenaIncendiaryTestPawn : public ACharacter, public IAbilitySystemInterface, public IArenaTeamAgentInterface
{
	GENERATED_BODY()

public:
	AArenaIncendiaryTestPawn();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	virtual FGenericTeamId GetGenericTeamId() const override { return Team; }

	virtual void SetGenericTeamId(const FGenericTeamId& InTeam) override
	{
		Team = InTeam;
	}

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UArenaHealthSet> Health;

	UPROPERTY()
	TObjectPtr<UArenaCombatSet> Combat;

	FGenericTeamId Team = FGenericTeamId(1);
};

/** A reversible modifier exercises the same lifecycle a frost/slow GE needs. */
UCLASS(Transient)
class UArenaGroundAreaTestSlowEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UArenaGroundAreaTestSlowEffect();
};
