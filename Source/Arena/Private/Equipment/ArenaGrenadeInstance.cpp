// Copyright Ludens Studio. All Rights Reserved.


#include "Equipment/ArenaGrenadeInstance.h"

UArenaGrenadeInstance::UArenaGrenadeInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const FGrenadeParams& UArenaGrenadeInstance::GetGrenadeParams() const
{
	return GrenadeParams;
}
