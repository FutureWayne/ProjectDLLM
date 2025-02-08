// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Grenade.h"

#include "Character/ArenaCharacter.h"
#include "Equipment/ArenaGrenadeInstance.h"
#include "System/ArenaSystemStatics.h"
#include "Weapon/ArenaGrenadeBase.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

UArenaGameplayAbility_Grenade::UArenaGameplayAbility_Grenade(const FObjectInitializer& ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UArenaGameplayAbility_Grenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UArenaEquipmentInstance* EquipmentInstance = GetAssociatedEquipment();
	GrenadeInstance = Cast<UArenaGrenadeInstance>(EquipmentInstance);
	
	checkf(GrenadeInstance, TEXT("UArenaGameplayAbility_Grenade::ActivateAbility: GrenadeInstance is nullptr."));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}


AArenaGrenadeBase* UArenaGameplayAbility_Grenade::SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation)
{
	const FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);
	AActor* Owner = GetOwningActorFromActorInfo();
	APawn* Instigator = GetArenaCharacterFromActorInfo();
	UArenaGrenadeDefinitionData* GrenadeDefinitionData = GrenadeInstance->GetGrenadeDefinitionData();
	
	AArenaGrenadeBase* RetGrenade = UArenaSystemStatics::SpawnGrenadeByGrenadeInstance(GetWorld(), SpawnTransform, GrenadeDefinitionData, Owner, Instigator);
	if (ensureMsgf(RetGrenade, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: OutGrenade is nullptr.")))
	{
		return RetGrenade;
	}
	
	return nullptr;
}

