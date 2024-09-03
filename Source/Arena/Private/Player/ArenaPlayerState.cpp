// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ArenaPlayerState.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"

AArenaPlayerState::AArenaPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("HealthSet"));
	
	NetUpdateFrequency = 100.0f;
}

UAbilitySystemComponent* AArenaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UArenaHealthSet* AArenaPlayerState::GetArenaHealthSet() const
{
	return ArenaHealthSet;
}

