// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Death.h"

#include "ArenaGameplayTags.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Character/ArenaHealthComponent.h"


UArenaGameplayAbility_Death::UArenaGameplayAbility_Death(const FObjectInitializer& ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	bAutoStartDeath = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = ArenaGameplayTags::GameplayEvent_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UArenaGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	UArenaAbilitySystemComponent* ArenaASC = CastChecked<UArenaAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(ArenaGameplayTags::Ability_Behavior_SurvivesDeath);

	// Cancel all abilities without survive death tag
	ArenaASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	SetCanBeCanceled(false);

	// Block other abilities from starting
	if (!ChangeActivationGroup(EArenaAbilityActivationGroup::Exclusive_Blocking))
	{
		UE_LOG(LogTemp, Error, TEXT("UArenaGameplayAbility_Death::ActivateAbility: Ability [%s] failed to change activation group to blocking."), *GetName());
	}

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UArenaGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	check(ActorInfo);
	
	// Always try to finish the death when the ability ends in case the ability doesn't.
	// This won't do anything if the death hasn't been started.
	FinishDeath();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UArenaGameplayAbility_Death::StartDeath()
{
	if (UArenaHealthComponent* HealthComponent = UArenaHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EArenaDeathState::NotDead)
		{
			HealthComponent->StartDeath();
		}
	}
}

void UArenaGameplayAbility_Death::FinishDeath()
{
	if (UArenaHealthComponent* HealthComponent = UArenaHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EArenaDeathState::DeathStarted)
		{
			HealthComponent->FinishDeath();
		}
	}
}
