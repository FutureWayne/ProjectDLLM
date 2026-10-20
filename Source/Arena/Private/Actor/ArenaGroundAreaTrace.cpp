// Copyright Ludens Studio. All Rights Reserved.

#include "Actor/ArenaGroundAreaTrace.h"

#include "Weapon/ArenaGrenadeBase.h"
#include "Actor/ArenaEffectActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"

bool ArenaGroundArea::ShouldIgnore(const AActor* Actor)
{
	return Actor && (Actor->IsA<APawn>() ||
		Actor->IsA<AArenaGrenadeBase>() ||
		Actor->IsA<AArenaEffectActor>() ||
		(Actor->ActorHasTag(TEXT("GroundArea.TraceIgnore")) ||
			Actor->ActorHasTag(TEXT("Incendiary.TraceIgnore"))));
}

bool ArenaGroundArea::Trace(const UWorld* World, FVector Start, FVector End, const AActor* Ignore, FHitResult& Hit)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GroundAreaSurface), true, Ignore);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);

	// Limit retries through ignored geometry; exhausting the limit keeps the last hit as an obstruction.
	for (int32 Retry = 0; Retry < 32; ++Retry)
	{
		if (!World->LineTraceSingleByObjectType(Hit, Start, End, Objects, Params))
		{
			return false;
		}

		if (ShouldIgnore(Hit.GetActor()))
		{
			Params.AddIgnoredActor(Hit.GetActor());
			continue;
		}

		if (Hit.GetComponent() &&
			(Hit.GetComponent()->ComponentHasTag(TEXT("GroundArea.TraceIgnore")) ||
				Hit.GetComponent()->ComponentHasTag(TEXT("Incendiary.TraceIgnore"))))
		{
			Params.AddIgnoredComponent(Hit.GetComponent());
			continue;
		}

		return true;
	}

	return true;
}
