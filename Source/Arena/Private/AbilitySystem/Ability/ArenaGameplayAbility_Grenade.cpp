// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Grenade.h"

#include "Character/ArenaCharacter.h"
#include "Equipment/ArenaGrenadeInstance.h"
#include "System/ArenaSystemStatics.h"
#include "Weapon/ArenaGrenadeBase.h"

UArenaGameplayAbility_Grenade::UArenaGameplayAbility_Grenade(const FObjectInitializer& ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UArenaGameplayAbility_Grenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UArenaEquipmentInstance* EquipmentInstance = GetAssociatedEquipment();
	GrenadeInstance = Cast<UArenaGrenadeInstance>(EquipmentInstance);
	
	ensureMsgf(GrenadeInstance, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: GrenadeInstance is nullptr."));
}


AArenaGrenadeBase* UArenaGameplayAbility_Grenade::SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation)
{
	const FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);
	AActor* Owner = GetOwningActorFromActorInfo();
	APawn* Instigator = GetArenaCharacterFromActorInfo();
	
	AArenaGrenadeBase* RetGrenade = UArenaSystemStatics::SpawnGrenadeByGrenadeInstance(GetWorld(), SpawnTransform, GrenadeInstance.GetClass(), Owner, Instigator);
	if (ensureMsgf(RetGrenade, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: OutGrenade is nullptr.")))
	{
		return RetGrenade;
	}
	
	return nullptr;
}

