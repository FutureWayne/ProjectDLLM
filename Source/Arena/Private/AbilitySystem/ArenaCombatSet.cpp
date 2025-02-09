// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/ArenaCombatSet.h"

#include "Net/UnrealNetwork.h"

UArenaCombatSet::UArenaCombatSet()
{
	BurstCount = 1.0f;
	SpeedBuff = 0.0f;
}

void UArenaCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UArenaCombatSet, BurstCount, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UArenaCombatSet, SpeedBuff, COND_None, REPNOTIFY_Always);
}

void UArenaCombatSet::OnRep_BurstCount(const FGameplayAttributeData& OldBurstCount)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UArenaCombatSet, BurstCount, OldBurstCount);
	OnBurstCountChanged.Broadcast(OldBurstCount.GetCurrentValue(), GetBurstCount(), nullptr);
}

void UArenaCombatSet::OnRep_SpeedBuff(const FGameplayAttributeData& OldSpeedBuff)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UArenaCombatSet, SpeedBuff, OldSpeedBuff);

	OnSpeedBuffChanged.Broadcast(OldSpeedBuff.GetCurrentValue(), GetSpeedBuff(), nullptr);
}

void UArenaCombatSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetBurstCountAttribute())
	{
		OnBurstCountChanged.Broadcast(OldValue, NewValue, nullptr);
	}
	
	else if (Attribute == GetSpeedBuffAttribute())
	{
		OnSpeedBuffChanged.Broadcast(OldValue, NewValue, nullptr);
	}
}

void UArenaCombatSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetBurstCountAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 5.f);
	}
	
	else if (Attribute == GetSpeedBuffAttribute())
	{
		NewValue = FMath::Clamp(NewValue, -100.0f, 10.f);
	}
}
