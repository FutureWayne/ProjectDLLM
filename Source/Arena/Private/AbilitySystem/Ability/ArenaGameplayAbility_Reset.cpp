// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Reset.h"

#include "ArenaGameplayTags.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Character/ArenaCharacter.h"
#include "GameFramework/GameplayMessageSubsystem.h"

UArenaGameplayAbility_Reset::UArenaGameplayAbility_Reset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// Add the ability trigger tag as default to the CDO.
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = ArenaGameplayTags::GameplayEvent_RequestReset;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UArenaGameplayAbility_Reset::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	UArenaAbilitySystemComponent* ArenaASC = CastChecked<UArenaAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	
	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(ArenaGameplayTags::Ability_Behavior_SurvivesDeath);

	// Cancel all abilities and block others from starting.
	ArenaASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	SetCanBeCanceled(false);

	if (AArenaCharacter* ArenaChar = Cast<AArenaCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		ArenaChar->Reset();
	}

	// Let others know a reset has occurred
	FArenaPlayerResetMessage Message;
	Message.OwnerPlayerState = CurrentActorInfo->OwnerActor.Get();
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(ArenaGameplayTags::GameplayEvent_Reset, Message);
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const bool bReplicateEndAbility = true;
	const bool bWasCanceled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCanceled);
}
