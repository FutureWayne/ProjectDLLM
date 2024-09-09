// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ArenaPlayerState.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"

AArenaPlayerState::AArenaPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("HealthSet"));
	
	NetUpdateFrequency = 100.0f;
}

void AArenaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaPlayerState, Team);
}

UAbilitySystemComponent* AArenaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UArenaHealthSet* AArenaPlayerState::GetArenaHealthSet() const
{
	return ArenaHealthSet;
}

void AArenaPlayerState::SetTeam(const ETeam NewTeam)
{
	Team = NewTeam;
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(NewTeam);
	}
}

void AArenaPlayerState::OnRep_Team()
{
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(Team);
	}
}

