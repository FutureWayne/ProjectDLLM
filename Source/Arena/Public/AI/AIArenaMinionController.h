// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIArenaMinionController.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AAIArenaMinionController : public AAIController
{
	GENERATED_BODY()
	
public:
	AAIArenaMinionController();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName FocusOnKeyName = "TargetToFollow";

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

private:
	AActor* GetFocusOnActor() const;
};
