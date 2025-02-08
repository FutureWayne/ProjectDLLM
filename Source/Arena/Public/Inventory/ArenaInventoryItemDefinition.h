// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArenaInventoryItemDefinition.generated.h"

class UArenaInventoryItemInstance;

//////////////////////////////////////////////////////////////////////


// Represents a fragment of an item definition
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class ARENA_API UArenaInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:	
	virtual void OnInstanceCreated(UArenaInventoryItemInstance* Instance, AActor* OwnerActor) {}
	
	virtual void OnInstanceRemoved(UArenaInventoryItemInstance* Instance, AActor* OwnerActor) {}
};

//////////////////////////////////////////////////////////////////////

/**
 * 
 */
UCLASS(Blueprintable, Const, Abstract)
class ARENA_API UArenaInventoryItemDefinition : public UObject
{
	GENERATED_BODY()
	
public:
	UArenaInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<UArenaInventoryItemFragment>> Fragments;

public:
	const UArenaInventoryItemFragment* FindFragmentByClass(const TSubclassOf<UArenaInventoryItemFragment>& FragmentClass) const;
};

UCLASS()
class ARENA_API UArenaInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UArenaInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UArenaInventoryItemDefinition> ItemDef, TSubclassOf<UArenaInventoryItemFragment> FragmentClass);
};
