// Copyright Ludens Studio. All Rights Reserved.

#include "Actor/ArenaEffectActor.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"

AArenaEffectActor::AArenaEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AArenaEffectActor::InitializeEffect_Implementation(const UArenaEffectActorDefinition* Definition)
{
}

void AArenaEffectActor::SetSource(APawn* InInstigator, UAbilitySystemComponent* InSourceAbilitySystem)
{
	SourceAbilitySystem = InSourceAbilitySystem
		? InSourceAbilitySystem
		: UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InInstigator);

	if (!SourceActor.IsValid() && SourceAbilitySystem.IsValid())
	{
		// For players this is the PlayerState, which outlives the Pawn and keeps team and attribution resolvable.
		SourceActor = SourceAbilitySystem->GetOwnerActor();
	}

	// The instigator link survives death but is cleared once the Pawn is collected.
	SetInstigator(InInstigator);
}

AActor* AArenaEffectActor::GetSourceActor() const
{
	return SourceActor.IsValid() ? SourceActor.Get() : GetInstigator();
}

// Called when the game starts or when spawned
void AArenaEffectActor::BeginPlay()
{
	Super::BeginPlay();

	// Spawners that predate SetSource still need a captured source before their instigator disappears.
	if (!SourceAbilitySystem.IsValid() && !SourceActor.IsValid() && GetInstigator())
	{
		SetSource(GetInstigator(), nullptr);
	}
}

void AArenaEffectActor::ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	UAbilitySystemComponent* InstigatorASC = GetSourceAbilitySystem();
	if (!InstigatorASC)
	{
		InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetSourceActor());
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (TargetASC == nullptr)
	{
		return;
	}

	// Some effect actors may not have an instigator, such as the respawn chamber
	if (InstigatorASC == nullptr)
	{
		InstigatorASC = TargetASC;
	}

	check(GameplayEffectClass);
	FGameplayEffectContextHandle EffectContext = InstigatorASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(GameplayEffectClass, 1, EffectContext);
	const FActiveGameplayEffectHandle ActiveEffectHandle = InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	const bool bIsInfinite = SpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	if (bIsInfinite && InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC);
	}

	if (!bIsInfinite)
	{
		Destroy();
	}
}

void AArenaEffectActor::OnOverlap(AActor* TargetActor)
{
	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}

	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}

	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
}

void AArenaEffectActor::OnEndOverlap(AActor* TargetActor)
{
	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}

	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}

	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}

	if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
		if (!IsValid(TargetASC))
		{
			return;
		}

		TArray<FActiveGameplayEffectHandle> HandlesToRemove;
		for (const TTuple<FActiveGameplayEffectHandle, UAbilitySystemComponent*> HandlePair : ActiveEffectHandles)
		{
			if (TargetASC == HandlePair.Value)
			{
				TargetASC->RemoveActiveGameplayEffect(HandlePair.Key, 1);
				HandlesToRemove.Add(HandlePair.Key);
			}
		}

		for (FActiveGameplayEffectHandle& Handle : HandlesToRemove)
		{
			ActiveEffectHandles.FindAndRemoveChecked(Handle);
		}
	}
}
