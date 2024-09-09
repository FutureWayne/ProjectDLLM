// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ArenaCharacterBase.h"

#include "EnhancedInputSubsystems.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Character/ArenaHealthComponent.h"

// Sets default values
AArenaCharacterBase::AArenaCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent = CreateDefaultSubobject<UArenaHealthComponent>(TEXT("HealthComponent"));
}

UArenaAbilitySystemComponent* AArenaCharacterBase::GetArenaAbilitySystemComponent() const
{
	return Cast<UArenaAbilitySystemComponent>(AbilitySystemComponent);
}

UAbilitySystemComponent* AArenaCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UArenaHealthSet* AArenaCharacterBase::GetArenaHealthSet() const
{
	return ArenaHealthSet;
}

// Called when the game starts or when spawned
void AArenaCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AArenaCharacterBase::AddCharacterAbilities() const
{
	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent();

	ArenaASC->AddCharacterAbilities(StartupAbilities);
}

// Called every frame
void AArenaCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

