// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ArenaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"

UArenaAttributeSet::UArenaAttributeSet()
{
}

UWorld* UArenaAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UArenaAbilitySystemComponent* UArenaAttributeSet::GetArenaAbilitySystemComponent() const
{
	return Cast<UArenaAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
