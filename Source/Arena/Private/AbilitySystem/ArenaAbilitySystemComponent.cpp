// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ArenaAbilitySystemComponent.h"

#include "AbilitySystem/ArenaGameplayAbility.h"

void UArenaAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);

			if (const UArenaGameplayAbility* ArenaAbility = Cast<UArenaGameplayAbility>(AbilitySpec.Ability))
			{
				AbilitySpec.DynamicAbilityTags.AddTag(ArenaAbility->StartupInputTag);
				GiveAbility(AbilitySpec);
			}
		}
	}
}

void UArenaAbilitySystemComponent::AbilityTagHeld(const FGameplayTag& InputTag)
{
}

void UArenaAbilitySystemComponent::AbilityTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);
			if (!AbilitySpec.IsActive())
			{
				TryActivateAbility(AbilitySpec.Handle);
			}
		}
	}
}

void UArenaAbilitySystemComponent::AbilityTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag) && AbilitySpec.IsActive())
		{
			AbilitySpecInputReleased(AbilitySpec);
		}
	}
}
