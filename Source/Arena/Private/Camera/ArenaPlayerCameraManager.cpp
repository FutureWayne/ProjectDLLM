// Copyright Ludens Studio. All Rights Reserved.

#include "Camera/ArenaPlayerCameraManager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/ArenaCameraComponent.h"
#include "Camera/ArenaUICameraManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaPlayerCameraManager)

class FDebugDisplayInfo;

static FName UICameraComponentName(TEXT("UICamera"));

AArenaPlayerCameraManager::AArenaPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = ARENA_CAMERA_DEFAULT_FOV;
	ViewPitchMin = ARENA_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = ARENA_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UArenaUICameraManagerComponent>(UICameraComponentName);
}

UArenaUICameraManagerComponent* AArenaPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void AArenaPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// If the UI Camera is looking at something, let it have priority.
	if (UICamera->NeedsToUpdateViewTarget())
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
		UICamera->UpdateViewTarget(OutVT, DeltaTime);
		return;
	}

	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void AArenaPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("ArenaPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UArenaCameraComponent* CameraComponent = UArenaCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}



