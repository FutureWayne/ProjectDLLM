// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ArenaAbilitySystemComponent.h"

#include "AbilitySystem/ArenaGameplayAbility.h"

UArenaAbilitySystemComponent::UArenaAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();

	FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}

void UArenaAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);

			if (const UArenaGameplayAbility* ArenaAbility = Cast<UArenaGameplayAbility>(AbilitySpec.Ability))
			{
				AbilitySpec.DynamicAbilityTags.AddTag(ArenaAbility->GetStartupInputTag());
				GiveAbility(AbilitySpec);
			}
		}
	}
}

void UArenaAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

void UArenaAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UArenaAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	// TODO: Clear ability input if has input blocked tag

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	//
	// Process all abilities that activate when the input is held.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability && !Spec->IsActive())
			{
				const UArenaGameplayAbility* ArenaAbilityCDO = Cast<UArenaGameplayAbility>(Spec->Ability);
				if (ArenaAbilityCDO && ArenaAbilityCDO->GetActivationPolicy() == EArenaAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.Add(SpecHandle);
				}
			}
		}
	}

	//
	// Process all abilities that had their input pressed this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability)
			{
				Spec->InputPressed = true;

				if (Spec->IsActive())
				{
					AbilitySpecInputPressed(*Spec);
				}
				else
				{
					const UArenaGameplayAbility* ArenaAbilityCDO = Cast<UArenaGameplayAbility>(Spec->Ability);
					if (ArenaAbilityCDO && ArenaAbilityCDO->GetActivationPolicy() == EArenaAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.Add(SpecHandle);
					}
				}
			}
		}
	}

	//
	// Try to activate all the abilities that are from presses and holds.
	// We do it all at once so that held inputs don't activate the ability
	// and then also send an input event to the ability because of the press.
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	//
	// Process all abilities that had their input released this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	//
	// Clear the cached ability handles.
	//
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UArenaAbilitySystemComponent::CancelAbilitiesByFunc(const TShouldCancelAbilityFunc& ShouldCancelFunc,
                                                         bool bReplicateCancelAbility)
{
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		UArenaGameplayAbility* ArenaAbilityCDO = Cast<UArenaGameplayAbility>(AbilitySpec.Ability);
		if (!ArenaAbilityCDO)
		{
			UE_LOG(LogTemp, Warning, TEXT("CancelAbilitiesByFunc: Non-ArenaGameplayAbility %s was Granted to ASC. Skipping."), *AbilitySpec.Ability.GetName());
			continue;
		}

		if (ArenaAbilityCDO->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced)
		{
			// Cancel all spawned instances
			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			for (UGameplayAbility* Instance : Instances)
			{
				UArenaGameplayAbility* ArenaAbilityInstance = Cast<UArenaGameplayAbility>(Instance);

				if (ShouldCancelFunc(ArenaAbilityInstance, AbilitySpec.Handle))
				{
					if (ArenaAbilityInstance->CanBeCanceled())
					{
						ArenaAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), ArenaAbilityInstance->GetCurrentActivationInfo(), bReplicateCancelAbility);
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."), *ArenaAbilityInstance->GetName());
					}
				}
			}
		}
		else
		{
			// Cancel the non-instanced ability CDO
			if (ShouldCancelFunc(ArenaAbilityCDO, AbilitySpec.Handle))
			{
				// Non-instanced abilities should always be cancelable
				check(ArenaAbilityCDO->CanBeCanceled());
				ArenaAbilityCDO->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), ArenaAbilityCDO->GetCurrentActivationInfo(), bReplicateCancelAbility);
			}
		}
	}
}

bool UArenaAbilitySystemComponent::IsActivationGroupBlocked(EArenaAbilityActivationGroup Group) const
{
	bool bBlocked = false;

	switch (Group)
	{
		case EArenaAbilityActivationGroup::Independent:
			// Independent abilities are never blocked.
			bBlocked = false;
			break;

		case EArenaAbilityActivationGroup::Exclusive_Replaceable:
		case EArenaAbilityActivationGroup::Exclusive_Blocking:
			bBlocked = ActivationGroupCounts[static_cast<uint8>(EArenaAbilityActivationGroup::Exclusive_Blocking)] > 0;
			break;

		default:
			checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), static_cast<uint8>(Group));
			break;
	}

	return bBlocked;
}

void UArenaAbilitySystemComponent::AddAbilityToActivationGroup(EArenaAbilityActivationGroup Group,
	UArenaGameplayAbility* ArenaAbility)
{
	check(ArenaAbility);
	check(ActivationGroupCounts[static_cast<uint8>(Group)] < INT32_MAX);

	ActivationGroupCounts[static_cast<uint8>(Group)]++;

	constexpr bool bReplicateCancelAbility = false;
	switch (Group)
	{
		case EArenaAbilityActivationGroup::Independent:
			// Independent abilities do not cancel any other abilities.
			break;

		case EArenaAbilityActivationGroup::Exclusive_Replaceable:
		case EArenaAbilityActivationGroup::Exclusive_Blocking:
			CancelActivationGroupAbilities(EArenaAbilityActivationGroup::Exclusive_Replaceable, ArenaAbility, bReplicateCancelAbility);
			break;

		default:
			checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), static_cast<uint8>(Group));
			break;
	}

	const int32 ExclusiveCount = ActivationGroupCounts[static_cast<uint8>(EArenaAbilityActivationGroup::Exclusive_Blocking)] + ActivationGroupCounts[static_cast<uint8>(EArenaAbilityActivationGroup::Exclusive_Replaceable)];
	if (ExclusiveCount > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
	}
}

void UArenaAbilitySystemComponent::RemoveAbilityFromActivationGroup(EArenaAbilityActivationGroup Group,
                                                                    const UArenaGameplayAbility* ArenaAbility)
{
	check(ArenaAbility);
	check(ActivationGroupCounts[static_cast<uint8>(Group)] > 0);

	ActivationGroupCounts[static_cast<uint8>(Group)]--;
}

void UArenaAbilitySystemComponent::CancelActivationGroupAbilities(EArenaAbilityActivationGroup Group,
	UArenaGameplayAbility* IgnoreArenaAbility, bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this, Group, IgnoreArenaAbility](const UArenaGameplayAbility* ArenaAbility, FGameplayAbilitySpecHandle Handle)
	{
		return ((ArenaAbility != IgnoreArenaAbility) && (ArenaAbility->GetActivationGroup() == Group));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UArenaAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	if (UArenaGameplayAbility* ArenaAbility = Cast<UArenaGameplayAbility>(Ability))
	{
		AddAbilityToActivationGroup(ArenaAbility->GetActivationGroup(), ArenaAbility);
	}
}


void UArenaAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability,
	bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	if (UArenaGameplayAbility* ArenaAbility = Cast<UArenaGameplayAbility>(Ability))
	{
		RemoveAbilityFromActivationGroup(ArenaAbility->GetActivationGroup(), ArenaAbility);
	}
}
