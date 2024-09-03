// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> DisplayText;
	
	void SetDisplayText(const FString& Text);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerLocalRole(APawn* InPawn);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerName(APawn* InPawn);

protected:
	virtual void NativeDestruct() override;
};
