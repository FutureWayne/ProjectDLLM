// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/ArenaGameplayAbility_FromEquip.h"
#include "ArenaGameplayAbility_FromGrenade.generated.h"

class UArenaGrenadeDefinitionData;
class AArenaGrenadeBase;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaGameplayAbility_FromGrenade : public UArenaGameplayAbility_FromEquip
{
	GENERATED_BODY()
	
public:
	UArenaGameplayAbility_FromGrenade(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Get Spawn Location"))
	FVector GetSpawnLocation();

	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Get Spawn Rotation"))
	FRotator GetSpawnRotation();

	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Get Grenade Definition Data"))
	const UArenaGrenadeDefinitionData* GetGrenadeDefinitionData() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float TraceDistance = 4500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float PredictionDistance = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	FVector SpawnLocationOffset = FVector(5.f, -25.f, 25.f);

private:
	static FRotator CalculateLaunchRotation(const UWorld* World, const FVector& Start, const FVector& Target, float LaunchSpeed, float GravityScale);
};
