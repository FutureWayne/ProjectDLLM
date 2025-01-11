// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_FromEquip.h"

#include "Equipment/ArenaEquipmentInstance.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaGameplayAbility_FromEquip)

UArenaGameplayAbility_FromEquip::UArenaGameplayAbility_FromEquip(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UArenaEquipmentInstance* UArenaGameplayAbility_FromEquip::GetAssociatedEquipment() const
{
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<UArenaEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

UArenaInventoryItemInstance* UArenaGameplayAbility_FromEquip::GetAssociatedItem() const
{
	if (UArenaEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<UArenaInventoryItemInstance>(Equipment->GetInstigator());
	}
	
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UArenaGameplayAbility_FromEquip::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
	{
		Context.AddError(NSLOCTEXT("Arena", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
