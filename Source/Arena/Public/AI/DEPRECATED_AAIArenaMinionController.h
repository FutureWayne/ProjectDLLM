// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DEPRECATED_AAIArenaMinionController.generated.h"

/**
 * 
 */
UCLASS(Deprecated)
class ARENA_API ADEPRECATED_AAIArenaMinionController : public AAIController
{
	GENERATED_BODY()
	
public:
	ADEPRECATED_AAIArenaMinionController();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FName FocusOnKeyName = "TargetToFollow";

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

private:
	AActor* GetFocusOnActor() const;
};
