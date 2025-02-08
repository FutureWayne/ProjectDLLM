// Copyright Ludens Studio. All Rights Reserved.


#include "Equipment/ArenaGrenadeInstance.h"
#include "Weapon/ArenaGrenadeBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UArenaGrenadeInstance::UArenaGrenadeInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const FGrenadeParams& UArenaGrenadeInstance::GetGrenadeParams() const
{
	return GrenadeParams;
}

#if WITH_EDITOR
EDataValidationResult UArenaGrenadeInstance::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (GrenadeParams.GrenadeClass == nullptr)
	{
		Context.AddError(NSLOCTEXT("Arena", "GrenadeClassNotSet", "Grenade class is not set."));
		Result = EDataValidationResult::Invalid;
	}
	
	return Result;
}
#endif
