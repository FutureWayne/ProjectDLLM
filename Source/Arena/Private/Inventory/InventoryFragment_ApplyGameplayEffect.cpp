// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/InventoryFragment_ApplyGameplayEffect.h"

#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"

void UInventoryFragment_ApplyGameplayEffect::OnInstanceCreated(UArenaInventoryItemInstance* Instance,
                                                               AActor* OwnerActor)
{
	if (!GameplayEffect || !OwnerActor)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	if (ASC)
	{
		// Apply the Gameplay Effect to the owner actor
		ActiveGameplayEffectHandle = ASC->ApplyGameplayEffectToSelf(GameplayEffect.GetDefaultObject(), 1.0f, ASC->MakeEffectContext());
	}
}

void UInventoryFragment_ApplyGameplayEffect::OnInstanceRemoved(UArenaInventoryItemInstance* Instance,
	AActor* OwnerActor)
{
	if (!GameplayEffect || !OwnerActor)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	if (ASC && ActiveGameplayEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ActiveGameplayEffectHandle, 1);
	}
}
