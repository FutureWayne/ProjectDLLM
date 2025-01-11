// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ArenaPlayerState.h"

#include "AbilitySystem/ArenaAbilitySet.h"
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
	
	SetNetUpdateFrequency(100.0f);
}

void AArenaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaPlayerState, MyTeamID);
}

UAbilitySystemComponent* AArenaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UArenaHealthSet* AArenaPlayerState::GetArenaHealthSet() const
{
	return ArenaHealthSet;
}

void AArenaPlayerState::AddAbilitySet(const UArenaAbilitySet* AbilitySet)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Remove any existing abilities
	AbilitySetHandles.RemoveAbilitiesFromAbilitySystem(AbilitySystemComponent);
	AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &AbilitySetHandles);
}


void AArenaPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	MyTeamID = NewTeamID;
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(GetTeam());
	}
}

void AArenaPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(GetTeam());
	}
}

