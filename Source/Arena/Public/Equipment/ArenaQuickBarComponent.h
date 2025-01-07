// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "ArenaQuickBarComponent.generated.h"

class UArenaInventoryItemInstance;
class UArenaEquipmentInstance;
class UArenaEquipmentManagerComponent;

/**
 * 
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class ARENA_API UArenaQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()
	
public:
	UArenaQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(Blueprintable, Category="Arena")
	void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable, Category="Arena")
	void CycleActiveSlotBackward();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Arena")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<UArenaInventoryItemInstance*> GetSlots() const
	{
		return Slots;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	UArenaInventoryItemInstance* GetActiveSlotItem() const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetNextFreeItemSlot() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItemToSlot(int32 SlotIndex, UArenaInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UArenaInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

	virtual void BeginPlay() override;

private:
	void UnequipItemInSlot();
	void EquipItemInSlot();

	UArenaEquipmentManagerComponent* FindEquipmentManager() const;

protected:
	UPROPERTY()
	int32 NumSlots = 3;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<TObjectPtr<UArenaInventoryItemInstance>> Slots;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UArenaEquipmentInstance> EquippedItem;
	
};