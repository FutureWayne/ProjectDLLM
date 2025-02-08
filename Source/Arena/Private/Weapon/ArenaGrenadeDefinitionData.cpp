// Copyright Ludens Studio. All Rights Reserved.


#include "Weapon/ArenaGrenadeDefinitionData.h"

#include "Weapon/ArenaGrenadeBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UArenaGrenadeDefinitionData::UArenaGrenadeDefinitionData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
EDataValidationResult UArenaGrenadeDefinitionData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (GrenadeClass == nullptr)
	{
		Context.AddError(NSLOCTEXT("Arena", "GrenadeClassNotSet", "Grenade class is not set."));
		Result = EDataValidationResult::Invalid;
	}
	
	return Result;
}
#endif
