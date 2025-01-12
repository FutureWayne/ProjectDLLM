// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaWeaponSpawner.generated.h"

struct FGameplayTag;
class UArenaInventoryItemDefinition;
class UCapsuleComponent;
class UArenaWeaponPickupDefinition;

UCLASS(Blueprintable,BlueprintType)
class ARENA_API AArenaWeaponSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArenaWeaponSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponDefinition, Category = "Arena|WeaponPickup")
	TObjectPtr<UArenaWeaponPickupDefinition> WeaponDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_WeaponAvailability, Category = "Arena|WeaponPickup")
	bool bIsWeaponAvailable;

	//The amount of time between weapon pickup and weapon spawning in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|WeaponPickup")
	float CoolDownTime;

	//Delay between when the weapon is made available and when we check for a pawn standing in the spawner. Used to give the bIsWeaponAvailable OnRep time to fire and play FX. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|WeaponPickup")
	float CheckExistingOverlapDelay;

	//Used to drive weapon respawn time indicators 0-1
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Arena|WeaponPickup")
	float CoolDownPercentage;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|WeaponPickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|WeaponPickup")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|WeaponPickup")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|WeaponPickup")
	FLinearColor PadColor;

	FTimerHandle CoolDownTimerHandle;
	FTimerHandle CheckOverlapsDelayTimerHandle;

public:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//Check for pawns standing on pad when the weapon is spawned. 
	void CheckForExistingOverlaps();

	UFUNCTION(BlueprintNativeEvent)
	void AttemptPickUpWeapon(APawn* Pawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|WeaponPickup")
	bool GiveWeapon(TSubclassOf<UArenaInventoryItemDefinition> WeaponItemClass, APawn* ReceivingPawn);

	void StartCoolDown();

	UFUNCTION(BlueprintCallable, Category = "Arena|WeaponPickup")
	void ResetCoolDown();

	UFUNCTION()
	void OnCoolDownTimerComplete();
	
	void SetWeaponPickupVisibility(bool bShouldBeVisible) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Arena|WeaponPickup")
	void PlayPickupEffects();

	UFUNCTION(BlueprintNativeEvent, Category = "Arena|WeaponPickup")
	void PlayRespawnEffects();

	UFUNCTION()
	void OnRep_WeaponAvailability();

	UFUNCTION()
	void OnRep_WeaponDefinition();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Arena|WeaponPickup")
	static int32 GetDefaultStatFromItemDef(const TSubclassOf<UArenaInventoryItemDefinition>& WeaponItemClass, FGameplayTag StatTag);
	
	void SetPickupDefinition(UArenaWeaponPickupDefinition* NewWeaponDefinition);
};
