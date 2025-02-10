// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/ArenaGameplayAbility_FromEquip.h"
#include "ArenaGameplayAbility_Grenade.generated.h"

class UArenaGrenadeDefinitionData;
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

	// ~Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	// ~End UGameplayAbility Interface
	
protected:
	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Spawn Grenade"))
	AArenaGrenadeBase* SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation);

	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Get Spawn Location"))
	FVector GetSpawnLocation();

	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Get Spawn Rotation"))
	FRotator GetSpawnRotation();

	UFUNCTION(BlueprintCallable, Category = "Grenade", meta = (DisplayName = "Get Target Location"))
	void BroadCastCooldownMessage();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UArenaGrenadeDefinitionData> GrenadeDefinitionData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float TraceDistance = 4500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float PredictionDistance = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	FVector SpawnLocationOffset = FVector(5.f, -25.f, 25.f);

private:
	static FRotator CalculateLaunchRotation(const UWorld* World, const FVector& Start, const FVector& Target, float LaunchSpeed, float GravityScale);

	void GetGrenadeDefinitionData();
private:
	FActiveGameplayEffectHandle CooldownEffectHandle;
};
