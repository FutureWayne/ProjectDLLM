// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Grenade.h"

#include "ArenaLogChannel.h"
#include "Character/ArenaCharacter.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Inventory/InventoryFragment_GrenadeDef.h"
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
	UArenaInventoryItemInstance* ItemInstance = GetAssociatedItem();
	if (!ItemInstance)
	{
		UE_LOG(LogArena, Error, TEXT("UArenaGameplayAbility_Grenade::ActivateAbility: ItemInstance is nullptr."));
		return;
	}

	const UInventoryFragment_GrenadeDef* GrenadeDef = ItemInstance->FindFragmentByClass<UInventoryFragment_GrenadeDef>();
	GrenadeDefinitionData = GrenadeDef->GetGrenadeDefinitionData();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

AArenaGrenadeBase* UArenaGameplayAbility_Grenade::SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation)
{
	const FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);
	AActor* Owner = GetOwningActorFromActorInfo();
	APawn* Instigator = GetArenaCharacterFromActorInfo();
	
	AArenaGrenadeBase* RetGrenade = UArenaSystemStatics::SpawnGrenadeByGrenadeInstance(GetWorld(), SpawnTransform, GrenadeDefinitionData, Owner, Instigator);
	if (ensureMsgf(RetGrenade, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: OutGrenade is nullptr.")))
	{
		return RetGrenade;
	}
	
	return nullptr;
}

