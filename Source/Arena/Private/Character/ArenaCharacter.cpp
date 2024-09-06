// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ArenaCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/ArenaHealthComponent.h"
#include "Player/ArenaPlayerState.h"
#include "UI/HUD/ArenaHUD.h"

AArenaCharacter::AArenaCharacter()
{
}

void AArenaCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Init ability actor info for the server
	InitAbilityActorInfo();
	AddCharacterAbilities();
}

void AArenaCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Init ability actor info for the client  
	InitAbilityActorInfo();
	AddCharacterAbilities();
}

void AArenaCharacter::InitAbilityActorInfo()
{
	AArenaPlayerState* PS = GetPlayerState<AArenaPlayerState>();
	check(PS);

	// PlayerState logically owns the AbilitySystemComponent and AttributeSet while the Character is the physical actor
	PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	AbilitySystemComponent = PS->GetAbilitySystemComponent();
	ArenaHealthSet = PS->GetArenaHealthSet();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AArenaHUD* HUD = Cast<AArenaHUD>(PC->GetHUD()))
		{
			HUD->InitOverlay(PC, PS, AbilitySystemComponent, ArenaHealthSet.Get());
		}
	}

	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent();
	HealthComponent->InitializeWithAbilitySystem(ArenaASC);
}
