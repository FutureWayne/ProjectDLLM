// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ArenaPlayerState.h"

#include "AbilitySystem/ArenaAbilitySet.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaCombatSet.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"

AArenaPlayerState::AArenaPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("HealthSet"));
	ArenaCombatSet = CreateDefaultSubobject<UArenaCombatSet>(TEXT("CombatSet"));
	
	SetNetUpdateFrequency(100.0f);
}

void AArenaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaPlayerState, MyTeamID);
	DOREPLIFETIME(AArenaPlayerState, StatTags);
}

UAbilitySystemComponent* AArenaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
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
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}

	
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(GetTeam());
	}
}

FGenericTeamId AArenaPlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnArenaTeamIndexChangedDelegate* AArenaPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AArenaPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AArenaPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AArenaPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AArenaPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void AArenaPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(GetTeam());
	}

	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

