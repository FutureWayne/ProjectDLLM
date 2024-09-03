// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/ArenaHUD.h"
#include "BlasterHUD.generated.h"

class UCharacterOverlay;
class UUserWidget;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<UTexture2D> CrosshairCenter;
	TWeakObjectPtr<UTexture2D> CrosshairLeft;
	TWeakObjectPtr<UTexture2D> CrosshairRight;
	TWeakObjectPtr<UTexture2D> CrosshairTop;
	TWeakObjectPtr<UTexture2D> CrosshairBottom;

	float CrosshairSpread;
	FLinearColor CrosshairColor;
};

/**
 * 
 */
UCLASS()
class ARENA_API ABlasterHUD : public AArenaHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;
	
private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, const FVector2D ViewportCenter, const FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float MaxCrosshairSpread = 16.f;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
