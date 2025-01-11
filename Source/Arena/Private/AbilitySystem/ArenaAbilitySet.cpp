// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/ArenaAbilitySet.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"

void FArenaAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FArenaAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FArenaAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* AttributeSet)
{
	GrantedAttributeSets.Add(AttributeSet);
}

void FArenaAbilitySet_GrantedHandles::TakeFromAbilitySystem(UArenaAbilitySystemComponent* ArenaASC)
{
	check(ArenaASC);

	if (!ArenaASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ArenaASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ArenaASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* AttributeSet : GrantedAttributeSets)
	{
		ArenaASC->RemoveSpawnedAttribute(AttributeSet);
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

void FArenaAbilitySet_GrantedHandles::RemoveAbilitiesFromAbilitySystem(UArenaAbilitySystemComponent* ArenaASC)
{
	check(ArenaASC);

	if (!ArenaASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ArenaASC->ClearAbility(Handle);
		}
	}

	AbilitySpecHandles.Reset();
}

UArenaAbilitySet::UArenaAbilitySet(const FObjectInitializer& ObjectInitializer)
{
}

void UArenaAbilitySet::GiveToAbilitySystem(UArenaAbilitySystemComponent* ArenaASC,
	FArenaAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(ArenaASC);

	if (!ArenaASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	// Grant the gameplay abilities.
	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FArenaAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];
		if (!IsValid(AbilityToGrant.Ability))
		{
			UE_LOG(LogTemp, Error, TEXT("UArenaAbilitySet::GiveToAbilitySystem: Invalid ability in ability set."));
			continue;
		}
		
		UArenaGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UArenaGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityToGrant.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityToGrant.InputTag);

		const FGameplayAbilitySpecHandle AbilitySpecHandle = ArenaASC->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	// Grant the gameplay effects.
	for (const FArenaAbilitySet_GameplayEffect& EffectToGrant : GrantedGameplayEffects)
	{
		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogTemp, Error, TEXT("UArenaAbilitySet::GiveToAbilitySystem: Invalid gameplay effect in ability set."));
			continue;
		}

		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle GameplayEffectHandle = ArenaASC->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, ArenaASC->MakeEffectContext());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}

	// Grant the attribute sets.
	for (const FArenaAbilitySet_AttributeSet& SetToGrant : GrantedAttributes)
	{
		if (!IsValid(SetToGrant.AttributeSet))
		{
			UE_LOG(LogTemp, Error, TEXT("UArenaAbilitySet::GiveToAbilitySystem: Invalid attribute set in ability set."));
			continue;
		}

		UAttributeSet* NewSet = NewObject<UAttributeSet>(ArenaASC->GetOwner(), SetToGrant.AttributeSet);
		ArenaASC->AddAttributeSetSubobject(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}
}
