// Copyright Ludens Studio. All Rights Reserved.


#include "Equipment/ArenaGrenadeInstance.h"


UArenaGrenadeInstance::UArenaGrenadeInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UArenaGrenadeDefinitionData* UArenaGrenadeInstance::GetGrenadeDefinitionData() const
{
	return GrenadeDefinitionData;
}
