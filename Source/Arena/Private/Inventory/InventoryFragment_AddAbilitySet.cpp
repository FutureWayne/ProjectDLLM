// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/InventoryFragment_AddAbilitySet.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Inventory/ArenaInventoryItemInstance.h"

void UInventoryFragment_AddAbilitySet::OnInstanceCreated(UArenaInventoryItemInstance* Instance,
                                                         AActor* OwnerActor)
{
	if (!IsValid(AbilitySetToApply) || !OwnerActor)
	{
		return;
	}

	UArenaAbilitySystemComponent* ArenaASC = Cast<UArenaAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor));
	if (ArenaASC)
	{
		AbilitySetToApply->GiveToAbilitySystem(Cast<UArenaAbilitySystemComponent>(ArenaASC), &Instance->GetGrantedHandles(), Instance);
	}
}

void UInventoryFragment_AddAbilitySet::OnInstanceRemoved(UArenaInventoryItemInstance* Instance,
	AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return;
	}

	UArenaAbilitySystemComponent* ArenaASC = Cast<UArenaAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor));
	if (ArenaASC)
	{
		Instance->GetGrantedHandles().TakeFromAbilitySystem(ArenaASC);
	}
}
