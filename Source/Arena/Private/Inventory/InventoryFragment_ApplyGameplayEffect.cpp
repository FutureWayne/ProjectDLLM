// Copyright Ludens Studio. All Rights Reserved.


#include "Inventory/InventoryFragment_ApplyGameplayEffect.h"

#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"

void UInventoryFragment_ApplyGameplayEffect::OnInstanceCreated(UArenaInventoryItemInstance* Instance,
                                                               AActor* OwnerActor)
{
	if (GameplayEffectsToApply.IsEmpty() || !OwnerActor)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	if (ASC)
	{
		// Apply all Gameplay Effects to the owner actor
		for (TSubclassOf<UGameplayEffect> GameplayEffectClass : GameplayEffectsToApply)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(OwnerActor);

			FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(GameplayEffectClass.GetDefaultObject(), 1.0f, ASC->MakeEffectContext());
			ActiveGameplayEffectHandles.Add(Handle);
		}
	}
}

void UInventoryFragment_ApplyGameplayEffect::OnInstanceRemoved(UArenaInventoryItemInstance* Instance,
	AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	if (ASC)
	{
		// Remove all Gameplay Effects from the owner actor
		for (FActiveGameplayEffectHandle Handle : ActiveGameplayEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle, 1);
		}
		ActiveGameplayEffectHandles.Empty();
	}
}
