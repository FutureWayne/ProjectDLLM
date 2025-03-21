// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/ArenaUICameraManagerComponent.h"

#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Camera/ArenaPlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaUICameraManagerComponent)

class AActor;
class FDebugDisplayInfo;

UArenaUICameraManagerComponent* UArenaUICameraManagerComponent::GetComponent(APlayerController* PC)
{
	if (PC != nullptr)
	{
		if (AArenaPlayerCameraManager* PCCamera = Cast<AArenaPlayerCameraManager>(PC->PlayerCameraManager))
		{
			return PCCamera->GetUICameraComponent();
		}
	}

	return nullptr;
}

UArenaUICameraManagerComponent::UArenaUICameraManagerComponent()
{
	bWantsInitializeComponent = true;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Register "showdebug" hook.
		if (!IsRunningDedicatedServer())
		{
			AHUD::OnShowDebugInfo.AddUObject(this, &ThisClass::OnShowDebugInfo);
		}
	}
}

void UArenaUICameraManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UArenaUICameraManagerComponent::SetViewTarget(AActor* InViewTarget, FViewTargetTransitionParams TransitionParams)
{
	TGuardValue<bool> UpdatingViewTargetGuard(bUpdatingViewTarget, true);

	ViewTarget = InViewTarget;
	CastChecked<AArenaPlayerCameraManager>(GetOwner())->SetViewTarget(ViewTarget, TransitionParams);
}

bool UArenaUICameraManagerComponent::NeedsToUpdateViewTarget() const
{
	return false;
}

void UArenaUICameraManagerComponent::UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime)
{
}

void UArenaUICameraManagerComponent::OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
}
