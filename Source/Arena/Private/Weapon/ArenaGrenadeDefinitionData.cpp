// Copyright Ludens Studio. All Rights Reserved.

#include "Weapon/ArenaGrenadeDefinitionData.h"

#include "Weapon/ArenaGrenadeBase.h"
#include "Actor/ArenaEffectActor.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "GameplayEffect.h"
#endif

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_DetonationPolicy_OnImpact, "DetonationPolicy.OnImpact", "Detonate on impact.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_DetonationPolicy_OnHitValidTarget, "DetonationPolicy.OnHitValidTarget", "Detonate when hit a target with valid ASC");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_DetonationPolicy_OnHitHorizontal, "DetonationPolicy.OnHitHorizontal", "Detonate when hit a horizontal surface.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GrenadeSlot_1, "GrenadeSlot.Slot1", "Tag used to destinguish grenade ability cooldowns");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GrenadeSlot_2, "GrenadeSlot.Slot2", "Tag used to destinguish grenade ability cooldowns");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GrenadeSlot_3, "GrenadeSlot.Slot3", "Tag used to destinguish grenade ability cooldowns");

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

	if (GrenadeSlotCooldownTag == FGameplayTag::EmptyTag)
	{
		Context.AddError(NSLOCTEXT("Arena", "GrenadeSlotCooldownTagNotSet", "Grenade slot cooldown tag is not set."));
		Result = EDataValidationResult::Invalid;
	}

	for (const FEffectActorSpawnData& Spawn : EffectActorsToSpawn)
	{
		if (Spawn.SpawnPolicy != ESpawnPolicy::DoNotSpawn &&
			(!Spawn.EffectActorClass || Spawn.SpawnCount < 0 || Spawn.SpawnCount > 64 || Spawn.SpawnRadius < 0.f))
		{
			Context.AddError(NSLOCTEXT("Arena", "EffectSpawnInvalid", "Effect spawn requires an actor class, a count between 0 and 64, and a non-negative spawn radius."));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif
