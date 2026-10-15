// Copyright Epic Games, Inc. All Rights Reserved.
#include "CommonSessionMenuSubsystem.h"
#include "CommonSessionSubsystem.h"
#include "CommonUserSubsystem.h"
#include "CommonUIExtensions.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "NativeGameplayTags.h"

bool UCommonSessionMenuSubsystem::Begin(UUserWidget* Menu)
{
	UCommonSessionSubsystem* Session = GetLocalPlayer()->GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>();
	if (!Menu || Menu->GetOwningLocalPlayer() != GetLocalPlayer() || !Session) { return false; }
	if (Sessions.IsValid() || Session->IsSessionBusy())
	{
		GetLocalPlayer()->GetGameInstance()->GetSubsystem<UCommonUserSubsystem>()->SendSystemMessage(FCommonUserTags::SystemMessage_Error,
			NSLOCTEXT("CommonGame", "SessionBusyTitle", "Session"), NSLOCTEXT("CommonGame", "SessionBusy", "Another session request is still in progress. Please wait."));
		return false;
	}
	Sessions = Session;
	SourceMenu = Menu;
	bRestoreEnabled = Menu->GetIsEnabled();
	// Subscribe before any request: OSS failures can complete synchronously.
	CreateHandle = Session->OnCreateSessionCompleteEvent.AddUObject(this, &ThisClass::Finish);
	JoinHandle = Session->OnJoinSessionCompleteEvent.AddUObject(this, &ThisClass::Finish);
	Menu->SetIsEnabled(false);
	InputToken = UCommonUIExtensions::SuspendInputForPlayer(GetLocalPlayer(), TEXT("SessionRequest"));
	if (const TSubclassOf<UCommonActivatableWidget> Class = ProgressWidgetClass.LoadSynchronous())
	{
		ProgressWidget = UCommonUIExtensions::PushContentToLayer_ForPlayer(GetLocalPlayer(), FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Modal")), Class);
	}
	return true;
}

void UCommonSessionMenuSubsystem::Host(UUserWidget* Menu, UCommonSession_HostSessionRequest* Request, bool bQuickPlay)
{
	if (!Begin(Menu)) { return; }
	if (bQuickPlay) { Sessions->QuickPlaySession(Menu->GetOwningPlayer(), Request); }
	else { Sessions->HostSession(Menu->GetOwningPlayer(), Request); }
}

void UCommonSessionMenuSubsystem::Join(UUserWidget* Menu, UCommonSession_SearchResult* Request)
{
	if (Begin(Menu)) { Sessions->JoinSession(Menu->GetOwningPlayer(), Request); }
}

void UCommonSessionMenuSubsystem::Finish(const FOnlineResultInformation& Result)
{
	ReleaseProgress();
	if (!Result.bWasSuccessful && Result.ErrorId != TEXT("Cancelled"))
	{
		GetLocalPlayer()->GetGameInstance()->GetSubsystem<UCommonUserSubsystem>()->SendSystemMessage(FCommonUserTags::SystemMessage_Error,
			NSLOCTEXT("CommonGame", "SessionFailedTitle", "Session"), Result.ErrorText);
	}
}

void UCommonSessionMenuSubsystem::ReleaseProgress()
{
	if (Sessions.IsValid())
	{
		Sessions->OnCreateSessionCompleteEvent.Remove(CreateHandle);
		Sessions->OnJoinSessionCompleteEvent.Remove(JoinHandle);
	}
	Sessions.Reset();
	if (ProgressWidget.IsValid()) { UCommonUIExtensions::PopContentFromLayer(ProgressWidget.Get()); }
	ProgressWidget.Reset();
	if (!InputToken.IsNone()) { UCommonUIExtensions::ResumeInputForPlayer(GetLocalPlayer(), InputToken); }
	InputToken = NAME_None;
	if (SourceMenu.IsValid()) { SourceMenu->SetIsEnabled(bRestoreEnabled); }
	SourceMenu.Reset();
}

void UCommonSessionMenuSubsystem::Deinitialize()
{
	ReleaseProgress();
	Super::Deinitialize();
}
