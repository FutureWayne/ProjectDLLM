// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ArenaMinion.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/ArenaHealthComponent.h"

AArenaMinion::AArenaMinion()
{
	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("AttributeSet"));
}

void AArenaMinion::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AddCharacterAbilities();
	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent();
	HealthComponent->InitializeWithAbilitySystem(ArenaASC);
}
