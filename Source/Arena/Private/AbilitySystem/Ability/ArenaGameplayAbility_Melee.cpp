// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Melee.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/ArenaCharacter.h"
#include "Kismet/KismetSystemLibrary.h"


void UArenaGameplayAbility_Melee::TraceMeleeTarget()
{
	bool bHit;
	TArray<FHitResult> HitResults;
	AArenaCharacter* ArenaCharacter = GetArenaCharacterFromActorInfo();
	FVector StartLocation = ArenaCharacter->GetActorLocation();
	FVector EndLocation = StartLocation + ArenaCharacter->GetActorForwardVector() * MeleeRange;
	float Radius = 96.0f;
	float HalfHeight = 48.0f;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(ArenaCharacter);
	bool bTraceComplex = false;

	bHit = UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		ArenaCharacter,
		StartLocation,
		EndLocation,
		Radius,
		HalfHeight,
		ObjectTypes,
		bTraceComplex,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		2.0f
	);

	if (bHit)
	{
		TSet<UAbilitySystemComponent*> TargetASCSet;

		// Gather ASC for hit targets
		for (FHitResult HitResult : HitResults)
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitResult.GetActor());
			if (!IsValid(TargetASC))
			{
				continue;
			}

			// TODO: Check if the hit actor and the instigator are in the same team

			// Check if hit actor is behind occluding geometry
			FHitResult OcclusionHitResult;
			ActorsToIgnore.Add(HitResult.GetActor());
			bool bOcclusionHit = UKismetSystemLibrary::LineTraceSingle(
				ArenaCharacter,
				StartLocation,
				HitResult.ImpactPoint,
				UEngineTypes::ConvertToTraceType(ECC_WorldDynamic),
				false,
				ActorsToIgnore,
				EDrawDebugTrace::ForDuration,
				OcclusionHitResult,
				true,
				FLinearColor::Red,
				FLinearColor::Green,
				0.5f
			);

			if (bOcclusionHit)
			{
				continue;
			}

			TargetASCSet.Add(TargetASC);
		}

		// Apply damage to hit targets
		check(MeleeDamageEffectClass);
		for (UAbilitySystemComponent* TargetASC : TargetASCSet)
		{
			UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
			const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(MeleeDamageEffectClass, GetAbilityLevel(), SourceASC->MakeEffectContext());
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}
