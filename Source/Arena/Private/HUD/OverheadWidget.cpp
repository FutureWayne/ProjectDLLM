// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(const FString& Text)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(Text));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	const ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;
	switch (RemoteRole)
	{
	case ROLE_Authority:
		Role = FString("Authority");
		break;
	case ROLE_AutonomousProxy:
		Role = FString("AutonomousProxy");
		break;
	case ROLE_SimulatedProxy:
		Role = FString("SimulatedProxy");
		break;
	default:
		Role = FString("None");
		break;
	}

	const FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
	SetDisplayText(RemoteRoleString);
}

void UOverheadWidget::ShowPlayerLocalRole(APawn* InPawn)
{
	const ENetRole LocalRole = InPawn->GetLocalRole();
	FString Role;
	switch (LocalRole)
	{
	case ROLE_Authority:
		Role = FString("Authority");
		break;
	case ROLE_AutonomousProxy:
		Role = FString("AutonomousProxy");
		break;
	case ROLE_SimulatedProxy:
		Role = FString("SimulatedProxy");
		break;
	default:
		Role = FString("None");
		break;
	}

	const FString LocalRoleString = FString::Printf(TEXT("Local Role: %s"), *Role);
	SetDisplayText(LocalRoleString);
}

void UOverheadWidget::ShowPlayerName(APawn* InPawn)
{
	if (APlayerState* PlayerState = InPawn->GetPlayerState())
	{
		const FString PlayerName = FString::Printf(TEXT("Name: %s"), *PlayerState->GetPlayerName());
		SetDisplayText(PlayerName);
	}
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
