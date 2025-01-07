// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/GameplayTagStack.h"

#include "ArenaInventoryItemInstance.generated.h"

class UArenaInventoryItemFragment;
class UArenaInventoryItemDefinition;
struct FGameplayTag;
/**
 * 
 */
UCLASS(BlueprintType)
class ARENA_API UArenaInventoryItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
	UArenaInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// ~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	// ~End of UObject interface

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category= Inventory)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;

	TSubclassOf<UArenaInventoryItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UArenaInventoryItemFragment* FindFragmentByClass(TSubclassOf<UArenaInventoryItemFragment> FragmentClass) const;

	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return static_cast<ResultClass*>(FindFragmentByClass(ResultClass::StaticClass()));
	}

private:
	void SetItemDef(const TSubclassOf<UArenaInventoryItemDefinition>& InItemDef);

	friend struct FArenaInventoryList;
	
private:
	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	UPROPERTY(Replicated)
	TSubclassOf<UArenaInventoryItemDefinition> ItemDef;
};
