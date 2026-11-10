// Copyright Ludens Studio. All Rights Reserved.

#include "AbilitySystem/Ability/ArenaGameplayAbility_GrenadeFire.h"

#include "AbilitySystemComponent.h"
#include "ArenaLogChannels.h"
#include "Character/ArenaCharacter.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayEffect.h"
#include "Messages/ArenaInteractionDurationMessage.h"
#include "System/ArenaSystemStatics.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"
#include "Equipment/ArenaEquipmentInstance.h"

UArenaGameplayAbility_GrenadeFire::UArenaGameplayAbility_GrenadeFire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UArenaGameplayAbility_GrenadeFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                                        const FGameplayEventData* TriggerEventData)
{
	if (!CheckCooldown(Handle, ActorInfo, nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

bool UArenaGameplayAbility_GrenadeFire::CheckCooldown(const FGameplayAbilitySpecHandle Handle,
                                                  const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UArenaGrenadeDefinitionData* GrenadeDefinitionData = GetGrenadeDefinitionData();
	if (GrenadeDefinitionData == nullptr)
	{
		UE_LOG(LogArenaAbilitySystem, Error, TEXT("UArenaGameplayAbility_Grenade::CheckCooldown: GrenadeDefinitionData is nullptr."));
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}
	
	FGameplayTag GrenadeSlotCooldownTag = GrenadeDefinitionData->GrenadeSlotCooldownTag;
	if (GrenadeSlotCooldownTag == FGameplayTag::EmptyTag)
	{
		UE_LOG(LogArenaAbilitySystem, Error, TEXT("UArenaGameplayAbility_Grenade::CheckCooldown: GrenadeSlotCooldownTag is empty. Using default cooldown"));
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	if (const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get())
	{
		return !AbilitySystemComponent->HasMatchingGameplayTag(GrenadeSlotCooldownTag);
	}

	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

bool UArenaGameplayAbility_GrenadeFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	if (bResult)
	{
		if (GetAssociatedEquipment() == nullptr)
		{
			UE_LOG(LogArenaAbilitySystem, Error, TEXT("Weapon ability %s cannot be activated because there is no associated ranged weapon (equipment instance=%s but needs to be derived from %s)"),
				*GetPathName(),
				*GetPathNameSafe(GetAssociatedEquipment()),
				*UArenaGameplayAbility_GrenadeFire::StaticClass()->GetName());
			bResult = false;
		}
	}

	return bResult;
}

bool UArenaGameplayAbility_GrenadeFire::CommitAbilityCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool ForceCooldown, FGameplayTagContainer* OptionalRelevantTags)
{
	BroadCastCooldownMessage();
	
	return Super::CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, ForceCooldown, OptionalRelevantTags);
}

void UArenaGameplayAbility_GrenadeFire::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	if (CooldownEffect == nullptr)
	{
		UE_LOG(LogArenaAbilitySystem, Error, TEXT("Cannot apply grenade cooldown for %s: cooldown effect is not configured."), *GetPathName());
		return;
	}

	const UArenaGrenadeDefinitionData* GrenadeDefinitionData = GetGrenadeDefinitionData();
	if (GrenadeDefinitionData == nullptr || !GrenadeDefinitionData->GrenadeSlotCooldownTag.IsValid())
	{
		UE_LOG(LogArenaAbilitySystem, Error, TEXT("Cannot apply grenade cooldown for %s: grenade definition or slot cooldown tag is missing."), *GetPathName());
		return;
	}

	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(CooldownEffect->GetClass(), 1.0f);
	if (CooldownSpec.IsValid())
	{
		FGameplayEffectSpec* CooldownSpecPtr = CooldownSpec.Data.Get();
		// The GE grants the shared cooldown marker; only this dynamic slot tag blocks this grenade.
		CooldownSpecPtr->DynamicGrantedTags.AddTag(GrenadeDefinitionData->GrenadeSlotCooldownTag);
		CooldownSpecPtr->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Cooldown.Grenade")), GrenadeDefinitionData->GrenadeAbilityCooldownTime);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
	}
}

AArenaGrenadeBase* UArenaGameplayAbility_GrenadeFire::SpawnGrenade(const FVector SpawnLocation, const FRotator SpawnRotation)
{
	const FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);
	AActor* Owner = GetOwningActorFromActorInfo();
	APawn* Instigator = GetArenaCharacterFromActorInfo();
	
	AArenaGrenadeBase* RetGrenade = UArenaSystemStatics::SpawnGrenadeByGrenadeDefinition(GetWorld(), SpawnTransform, GetGrenadeDefinitionData(), Owner, Instigator);
	if (ensureMsgf(RetGrenade, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: OutGrenade is nullptr.")))
	{
		return RetGrenade;
	}
	
	return nullptr;
}


void UArenaGameplayAbility_GrenadeFire::BroadCastCooldownMessage()
{
	const UArenaGrenadeDefinitionData* GrenadeDefinitionData = GetGrenadeDefinitionData();
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(GetWorld());
	FArenaInteractionDurationMessage Message;
	Message.Duration = GrenadeDefinitionData->GrenadeAbilityCooldownTime;
	Message.CooldownTag = GrenadeDefinitionData->GrenadeSlotCooldownTag;
	Message.Instigator = GetArenaCharacterFromActorInfo();
	MessageSubsystem.BroadcastMessage(ArenaGameplayTags::Ability_Grenade_Duration_Message, Message);
}
