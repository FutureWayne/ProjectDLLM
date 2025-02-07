// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/ArenaGameplayAbility_FromEquip.h"
#include "Equipment/ArenaGrenadeInstance.h"
#include "ArenaGameplayAbility_Grenade.generated.h"

class AArenaGrenadeBase;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaGameplayAbility_Grenade : public UArenaGameplayAbility_FromEquip
{
	GENERATED_BODY()
	
public:
	UArenaGameplayAbility_Grenade(const FObjectInitializer& ObjectInitializer);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Spawn Grenade"))
	AArenaGrenadeBase* SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	FGrenadeParams GrenadeParams;
};
