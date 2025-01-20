// Copyright Ludens Studio. All Rights Reserved.


#include "Actor/ArenaLootBoxData.h"

UArenaWeaponPickupDefinition* UArenaLootBoxData::GetRandomPickupDefinition() const
{
	// Ensure there's valid data in the array
	if (LootSpawningData.Num() == 0)
	{
		return nullptr;
	}

	// Calculate the total spawn chance
	float TotalChance = 0.0f;
	for (const FLootSpawningData& LootData : LootSpawningData)
	{
		TotalChance += LootData.SpawnChance;
	}

	// Generate a random number between 0 and TotalChance
	float RandomRoll = FMath::RandRange(0.0f, TotalChance);

	// Iterate through the array and determine the weapon to spawn
	float CumulativeChance = 0.0f;
	for (const FLootSpawningData& LootData : LootSpawningData)
	{
		CumulativeChance += LootData.SpawnChance;
		if (RandomRoll <= CumulativeChance)
		{
			return LootData.PickupDefinition;
		}
	}
	
	return nullptr;
}
