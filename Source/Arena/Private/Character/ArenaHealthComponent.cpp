// Copyright Ludens Studio. All Rights Reserved.


#include "Character/ArenaHealthComponent.h"

#include "ArenaGameplayTags.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UArenaHealthComponent::UArenaHealthComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	DeathState = EArenaDeathState::NotDead;
}

void UArenaHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UArenaHealthComponent, DeathState);
}

void UArenaHealthComponent::InitializeWithAbilitySystem(UArenaAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ArenaHealthComponent: Health component for owner %s already initialized with an ability system component."), *Owner->GetName());
		return;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ArenaHealthComponent: Cannot initialize health component for owner %s with a null ability system component."), *Owner->GetName());
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<UArenaHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogTemp, Error, TEXT("ArenaHealthComponent: Cannot initialize health component for owner %s without a health set."), *Owner->GetName());
		return;
	}

	HealthSet->OnHealthChanged.AddUObject(this, &UArenaHealthComponent::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &UArenaHealthComponent::HandleMaxHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &UArenaHealthComponent::HandleOutOfHealth);

	AbilitySystemComponent->SetNumericAttributeBase(UArenaHealthSet::GetHealthAttribute(), HealthSet->GetHealth());

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetMaxHealth(), HealthSet->GetMaxHealth(), nullptr);
}

void UArenaHealthComponent::UninitializeFromAbilitySystem()
{
	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
}

float UArenaHealthComponent::GetHealth() const
{
	return HealthSet ? HealthSet->GetHealth() : 0.0f;
}

float UArenaHealthComponent::GetMaxHealth() const
{
	return HealthSet ? HealthSet->GetMaxHealth() : 0.0f;
}

float UArenaHealthComponent::GetHealthNormalized() const
{
	if (HealthSet)
	{
		const float Health = HealthSet->GetHealth();
		const float MaxHealth = HealthSet->GetMaxHealth();

		return (MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f;
	}

	return 0.0f;
}

void UArenaHealthComponent::StartDeath()
{
	if (DeathState != EArenaDeathState::NotDead)
	{
		return;
	}

	DeathState = EArenaDeathState::DeathStarted;

	// Clear dying tag
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(ArenaGameplayTags::Status_Death_Dying, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathStarted.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UArenaHealthComponent::FinishDeath()
{
	if (DeathState != EArenaDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EArenaDeathState::DeathFinished;

	// Clear dying tag
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(ArenaGameplayTags::Status_Death_Dead, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathFinished.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UArenaHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	// TODO: Add Damage GE to owner
}

void UArenaHealthComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();
	
	Super::OnUnregister();
}

void UArenaHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(ArenaGameplayTags::Status_Death_Dying, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(ArenaGameplayTags::Status_Death_Dead, 0);
	}
}

void UArenaHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser,
                                                const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UArenaHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UArenaHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent && DamageEffectSpec)
	{
		// Send the "GameplayEvent.Death" gameplay event through the owner's ability system.  This can be used to trigger a death gameplay ability.
		{
			FGameplayEventData Payload;
			Payload.EventTag = ArenaGameplayTags::GameplayEvent_Death;
			Payload.Instigator = DamageInstigator;
			Payload.Target = AbilitySystemComponent->GetAvatarActor();
			Payload.OptionalObject = DamageEffectSpec->Def;
			Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
			Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
			Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
			Payload.EventMagnitude = DamageMagnitude;

			FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
			AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		}
	}
#endif
}

void UArenaHealthComponent::OnRep_DeathState(EArenaDeathState OldDeathState)
{
	const EArenaDeathState NewDeathState = DeathState;

	DeathState = OldDeathState;

	if (OldDeathState > NewDeathState)
	{
		// The server is trying to set us back but we've already predicted past the server state.
		UE_LOG(LogTemp, Warning, TEXT("ArenaHealthComponent: Predicted past server death state [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		return;
	}

	if (OldDeathState == EArenaDeathState::NotDead)
	{
		if (NewDeathState == EArenaDeathState::DeathStarted)
		{
			StartDeath();
		}
		else if (NewDeathState == EArenaDeathState::DeathFinished)
		{
			StartDeath();
			FinishDeath();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ArenaHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}

	else if (OldDeathState == EArenaDeathState::DeathStarted)
	{
		if (NewDeathState == EArenaDeathState::DeathFinished)
		{
			FinishDeath();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ArenaHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}

	ensureMsgf((DeathState == NewDeathState), TEXT("ArenaHealthComponent: Death transition failed [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
}

