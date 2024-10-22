// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ArenaCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/ArenaHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/ArenaPlayerState.h"
#include "UI/HUD/ArenaHUD.h"

AArenaCharacter::AArenaCharacter()
{
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);
}

void AArenaCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Init ability actor info for the server
	InitAbilityActorInfo();
	
}

void AArenaCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Init ability actor info for the client  
	InitAbilityActorInfo();
}

void AArenaCharacter::OnDeathStarted(AActor* OwningActor)
{
	DisableMovementAndCollision();
}

void AArenaCharacter::OnDeathFinished(AActor* OwningActor)
{
	DestroyDueToDeath();
}

void AArenaCharacter::DisableMovementAndCollision()
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	check(MovementComp);
	MovementComp->StopMovementImmediately();
	MovementComp->DisableMovement();
}

void AArenaCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();
	
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	SetActorHiddenInGame(true);
}

void AArenaCharacter::OnPlayerStateInitialized()
{
	AArenaPlayerState* PS = GetPlayerState<AArenaPlayerState>();
	check(PS);

	// Set the pointer to the player state
	ArenaPlayerState = PS;
}

void AArenaCharacter::InitAbilityActorInfo()
{
	AArenaPlayerState* PS = GetPlayerState<AArenaPlayerState>();
	if (PS == nullptr)
	{
		return;
	}

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
	
	AddCharacterAbilities();
}
