// Copyright Ludens Studio. All Rights Reserved.


#include "Equipment/ArenaEquipmentDefinition.h"
#include "Equipment/ArenaEquipmentInstance.h"


UArenaEquipmentDefinition::UArenaEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
{
	InstanceType = UArenaEquipmentInstance::StaticClass();
}
