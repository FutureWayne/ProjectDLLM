// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "ArenaNumberPopComponent.generated.h"

USTRUCT(BlueprintType)
struct FArenaNumberPopRequest
{
	GENERATED_BODY()

	// The world location to create the number pop at
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Number Pops")
	FVector WorldLocation;

	// Tags related to the source/cause of the number pop (for determining a style)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Number Pops")
	FGameplayTagContainer SourceTags;

	// Tags related to the target of the number pop (for determining a style)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arena|Number Pops")
	FGameplayTagContainer TargetTags;

	// The number to display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Number Pops")
	int32 NumberToDisplay = 0;

	FArenaNumberPopRequest()
		: WorldLocation(ForceInitToZero)
	{
	}
};

/**
 * 
 */
UCLASS(Abstract)
class ARENA_API UArenaNumberPopComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UArenaNumberPopComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Number Pop")
	virtual void AddNumberPop(const FArenaNumberPopRequest& NewRequest) {}
};
