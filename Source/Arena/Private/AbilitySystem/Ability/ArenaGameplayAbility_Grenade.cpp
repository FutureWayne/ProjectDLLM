// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Grenade.h"

#include "Character/ArenaCharacter.h"
#include "Equipment/ArenaGrenadeInstance.h"
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
	const UArenaGrenadeInstance* GrenadeInstance = Cast<UArenaGrenadeInstance>(EquipmentInstance);
	
	if (ensureMsgf(GrenadeInstance, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: GrenadeInstance is nullptr.")))
	{
		GrenadeParams = GrenadeInstance->GetGrenadeParams();
	}
}


AArenaGrenadeBase* UArenaGameplayAbility_Grenade::SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation)
{
	const FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetArenaCharacterFromActorInfo();
	SpawnParams.Instigator = GetArenaCharacterFromActorInfo();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
	AArenaGrenadeBase* OutGrenade = GetWorld()->SpawnActorDeferred<AArenaGrenadeBase>(GrenadeParams.GrenadeClass, SpawnTransform, SpawnParams.Owner, SpawnParams.Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (ensureMsgf(OutGrenade, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: Grenade is nullptr.")))
	{
		OutGrenade->SetGrenadeParameter(GrenadeParams);
		OutGrenade->FinishSpawning(SpawnTransform);
		return OutGrenade;
	}

	return nullptr;
}

