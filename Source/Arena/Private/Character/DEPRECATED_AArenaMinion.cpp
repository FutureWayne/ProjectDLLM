// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DEPRECATED_AArenaMinion.h"

#include "AbilitySystem/ArenaAbilitySet.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/ArenaHealthComponent.h"
#include "Net/UnrealNetwork.h"

ADEPRECATED_AArenaMinion::ADEPRECATED_AArenaMinion()
{
	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("AttributeSet"));
}

void ADEPRECATED_AArenaMinion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADEPRECATED_AArenaMinion, MyTeamID);
}

void ADEPRECATED_AArenaMinion::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	MyTeamID = NewTeamID;
}

void ADEPRECATED_AArenaMinion::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent();
	HealthComponent->InitializeWithAbilitySystem(ArenaASC);

	check(AbilitySet);
	UArenaAbilitySet* SetPtr = AbilitySet.Get();
	SetPtr->GiveToAbilitySystem(ArenaASC, nullptr);
}

	
