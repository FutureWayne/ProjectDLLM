// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/ArenaGameplayAbility.h"

#include "ArenaGameplayTags.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Character/ArenaCharacterBase.h"
#include "PlayerController/ArenaPlayerController.h"

#define ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(FunctionName, ReturnValue)																				    \
{																																						    \
	if (!ensure(IsInstantiated()))																														    \
	{																																					    \
		UE_LOG(LogTemp, Error, TEXT("%s: " #FunctionName " cannot be called on a non-instanced ability. Check the instancing policy."), *GetPathName());	\
		return ReturnValue;																																    \
	}																																					    \
}

UArenaGameplayAbility::UArenaGameplayAbility(const FObjectInitializer& ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EArenaAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EArenaAbilityActivationGroup::Independent;
}

AController* UArenaGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}

		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (const APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}

	return nullptr;	
}

AArenaCharacterBase* UArenaGameplayAbility::GetArenaCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AArenaCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

UArenaAbilitySystemComponent* UArenaGameplayAbility::GetArenaAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UArenaAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AArenaPlayerController* UArenaGameplayAbility::GetArenaPlayerControllerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AArenaPlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

bool UArenaGameplayAbility::CanChangeActivationGroup(EArenaAbilityActivationGroup NewGroup) const
{
	if (!IsInstantiated() || !IsActive())
	{
		return false;
	}

	if (ActivationGroup == NewGroup)
	{
		return false;
	}

	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponentFromActorInfo();
	check(ArenaASC);

	if ((ActivationGroup != EArenaAbilityActivationGroup::Exclusive_Blocking) && ArenaASC->IsActivationGroupBlocked(ActivationGroup))
	{
		// This ability can't change groups if it's blocked (unless it is the one doing the blocking).
		return false;
	}

	if ((NewGroup == EArenaAbilityActivationGroup::Exclusive_Replaceable) && !CanBeCanceled())
	{
		// This ability can't become replaceable if it can't be canceled.
		return false;
	}

	return true;
}

bool UArenaGameplayAbility::ChangeActivationGroup(EArenaAbilityActivationGroup NewGroup)
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ChangeActivationGroup, false);

	if (!CanChangeActivationGroup(NewGroup))
	{
		return false;
	}

	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponentFromActorInfo();
	check(ArenaASC);

	ArenaASC->RemoveAbilityFromActivationGroup(ActivationGroup, this);
	ArenaASC->AddAbilityToActivationGroup(NewGroup, this);

	ActivationGroup = NewGroup;

	return true;
}

void UArenaGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec) const
{
	const bool bIsPredicting = (Spec.ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting);

	if (ActorInfo && !Spec.IsActive() && !bIsPredicting && (ActivationPolicy == EArenaAbilityActivationPolicy::OnSpawn))
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		// If avatar actor is torn off or about to die, don't try to activate until we get the new one.
		if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
		{
			const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
			const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

			const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
			const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;
			
			if (bClientShouldActivate || bServerShouldActivate)
			{
				ASC->TryActivateAbility(Spec.Handle);
			}
		}
	}
}

bool UArenaGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                               const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	UArenaAbilitySystemComponent* ArenaASC = Cast<UArenaAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	check(ArenaASC);
	if (ArenaASC->IsActivationGroupBlocked(ActivationGroup))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(ArenaGameplayTags::Ability_ActivateFail_ActivationGroup);
		}
		return false;
	}

	return true;
}

void UArenaGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled)
{
	// The ability can not block canceling if it's replaceable.
	if (!bCanBeCanceled && (ActivationGroup == EArenaAbilityActivationGroup::Exclusive_Replaceable))
	{
		UE_LOG(LogTemp, Error, TEXT("SetCanBeCanceled: Ability [%s] can not block canceling because its activation group is replaceable."), *GetName());
		return;
	}

	Super::SetCanBeCanceled(bCanBeCanceled);
}

void UArenaGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}
