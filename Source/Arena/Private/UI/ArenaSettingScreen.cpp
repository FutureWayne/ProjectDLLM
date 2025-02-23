// Copyright Ludens Studio. All Rights Reserved.


#include "UI/ArenaSettingScreen.h"

#include "Input/CommonUIInputTypes.h"
#include "Player/ArenaLocalPlayer.h"
#include "Settings/ArenaGameSettingRegistry.h"


class UGameSettingRegistry;

void UArenaSettingScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BackHandle = RegisterUIActionBinding(FBindUIActionArgs(BackInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleBackAction)));
	ApplyHandle = RegisterUIActionBinding(FBindUIActionArgs(ApplyInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleApplyAction)));
	CancelChangesHandle = RegisterUIActionBinding(FBindUIActionArgs(CancelChangesInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleCancelChangesAction)));
}

UGameSettingRegistry* UArenaSettingScreen::CreateRegistry()
{
	UArenaGameSettingRegistry* NewRegistry = NewObject<UArenaGameSettingRegistry>();
	
	if (UArenaLocalPlayer* LocalPlayer = CastChecked<UArenaLocalPlayer>(GetOwningLocalPlayer()))
	{
		NewRegistry->Initialize(LocalPlayer);
	}
	
	return NewRegistry;
}

void UArenaSettingScreen::HandleBackAction()
{
	if (AttemptToPopNavigation())
	{
		return;
	}

	ApplyChanges();

	DeactivateWidget();
}

void UArenaSettingScreen::HandleApplyAction()
{
	ApplyChanges();
}

void UArenaSettingScreen::HandleCancelChangesAction()
{
	CancelChanges();
}

void UArenaSettingScreen::OnSettingsDirtyStateChanged_Implementation(bool bSettingsDirty)
{
	if (bSettingsDirty)
	{
		if (!GetActionBindings().Contains(ApplyHandle))
		{
			AddActionBinding(ApplyHandle);
		}
		if (!GetActionBindings().Contains(CancelChangesHandle))
		{
			AddActionBinding(CancelChangesHandle);
		}
	}
	else
	{
		RemoveActionBinding(ApplyHandle);
		RemoveActionBinding(CancelChangesHandle);
	}
}
