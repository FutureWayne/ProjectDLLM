// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ArenaAbilitySystemComponent.h"

#include "AbilitySystem/ArenaGameplayAbility.h"
#include "AbilitySystem/ArenaGlobalAbilitySystem.h"

UArenaAbilitySystemComponent::UArenaAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();

	FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}

void UArenaAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UArenaGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UArenaGlobalAbilitySystem>(GetWorld()))
	{
		GlobalAbilitySystem->UnregisterASC(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void UArenaAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor.Get());
	
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		// Notify all abilities that a new pawn avatar has been set
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			ensureMsgf(AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced, TEXT("InitAbilityActorInfo: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));

			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			for (UGameplayAbility* AbilityInstance : Instances)
			{
				UArenaGameplayAbility* ArenaAbilityInstance = CastChecked<UArenaGameplayAbility>(AbilityInstance);
				ArenaAbilityInstance->OnPawnAvatarSet();
			}
		}

		// Register with the global system once we actually have a pawn avatar. We wait until this time since some globally-applied effects may require an avatar.
		if (UArenaGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UArenaGlobalAbilitySystem>(GetWorld()))
		{
			GlobalAbilitySystem->RegisterASC(this);
		}

		TryActivateAbilitiesOnSpawn();
	}
}

void UArenaAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const UArenaGameplayAbility* ArenaAbilityCDO = Cast<UArenaGameplayAbility>(AbilitySpec.Ability))
		{
			ArenaAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
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
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
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
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
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

PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ensureMsgf(AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced, TEXT("CancelAbilitiesByFunc: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
			
		// Cancel all the spawned instances.
		TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
		for (UGameplayAbility* AbilityInstance : Instances)
		{
			UArenaGameplayAbility* ArenaAbilityInstance = CastChecked<UArenaGameplayAbility>(AbilityInstance);

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

void UArenaAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);
	
	if (Spec.IsActive())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
	}
}

void UArenaAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (Spec.IsActive())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		
		// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

