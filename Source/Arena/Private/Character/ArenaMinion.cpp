// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ArenaMinion.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/ArenaHealthComponent.h"
#include "Net/UnrealNetwork.h"

AArenaMinion::AArenaMinion()
{
	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("AttributeSet"));
}

void AArenaMinion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaMinion, MyTeamID);
}

void AArenaMinion::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	MyTeamID = NewTeamID;
}

void AArenaMinion::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AddCharacterAbilities();
	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent();
	HealthComponent->InitializeWithAbilitySystem(ArenaASC);
}
