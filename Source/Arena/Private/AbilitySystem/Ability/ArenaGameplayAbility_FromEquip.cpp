// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_FromEquip.h"

#include "Equipment/ArenaEquipmentInstance.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "ArenaLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaGameplayAbility_FromEquip)

UArenaGameplayAbility_FromEquip::UArenaGameplayAbility_FromEquip(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UArenaEquipmentInstance* UArenaGameplayAbility_FromEquip::GetAssociatedEquipment() const
{
	FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (Spec == nullptr)
	{
		UE_LOG(LogArenaAbilitySystem, Error, TEXT("UArenaGameplayAbility_FromEquip::GetAssociatedEquipment: Could not get ability spec"));
		return nullptr;
	}

	UArenaEquipmentInstance* Equipment = Cast<UArenaEquipmentInstance>(Spec->SourceObject.Get());
	if (Equipment == nullptr)
	{
		UE_LOG(LogArenaAbilitySystem, Error, TEXT("UArenaGameplayAbility_FromEquip::GetAssociatedEquipment: Could not get associated equipment"));
		return nullptr;
	}

	return Equipment;
}

UArenaInventoryItemInstance* UArenaGameplayAbility_FromEquip::GetAssociatedItem() const
{
	if (UArenaEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<UArenaInventoryItemInstance>(Equipment->GetInstigator());
	}

	else
	{
		UE_LOG(LogArenaAbilitySystem, Error, TEXT("UArenaGameplayAbility_FromEquip::GetAssociatedItem: Could not get associated equipment"));
		return nullptr;
	}
}

#if WITH_EDITOR
EDataValidationResult UArenaGameplayAbility_FromEquip::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	{
		Context.AddError(NSLOCTEXT("Arena", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
