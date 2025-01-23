// Copyright Ludens Studio. All Rights Reserved.


#include "Phases/ArenaGamePhaseAbility.h"

#include "AbilitySystemComponent.h"
#include "Phases/ArenaGamePhaseSubsystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaGamePhaseAbility)

#define LOCTEXT_NAMESPACE "UArenaGamePhaseAbility"


UArenaGamePhaseAbility::UArenaGamePhaseAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
}

void UArenaGamePhaseAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (ActorInfo->IsNetAuthority())
	{
		UWorld* World = ActorInfo->AbilitySystemComponent->GetWorld();
		UArenaGamePhaseSubsystem* PhaseSubsystem = UWorld::GetSubsystem<UArenaGamePhaseSubsystem>(World);
		PhaseSubsystem->OnBeginPhase(this, Handle);
	}
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UArenaGamePhaseAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo->IsNetAuthority())
	{
		UWorld* World = ActorInfo->AbilitySystemComponent->GetWorld();
		UArenaGamePhaseSubsystem* PhaseSubsystem = UWorld::GetSubsystem<UArenaGamePhaseSubsystem>(World);
		PhaseSubsystem->OnEndPhase(this, Handle);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

#if WITH_EDITOR
EDataValidationResult UArenaGamePhaseAbility::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!GamePhaseTag.IsValid())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(LOCTEXT("GamePhaseTagNotSet", "GamePhaseTag must be set to a tag representing the current phase."));
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
