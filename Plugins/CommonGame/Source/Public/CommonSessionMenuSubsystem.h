// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "CommonUserTypes.h"
#include "CommonSessionMenuSubsystem.generated.h"

class UUserWidget;
class UCommonActivatableWidget;
class UCommonSessionSubsystem;
class UCommonSession_HostSessionRequest;
class UCommonSession_SearchResult;

/** Owns menu progress independently of widgets that can be destroyed during travel. */
UCLASS(Config=Game)
class COMMONGAME_API UCommonSessionMenuSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	void Host(UUserWidget* Menu, UCommonSession_HostSessionRequest* Request, bool bQuickPlay);
	void Join(UUserWidget* Menu, UCommonSession_SearchResult* Request);
	virtual void Deinitialize() override;
private:
	bool Begin(UUserWidget* Menu);
	void Finish(const FOnlineResultInformation& Result);
	void ReleaseProgress();
	UPROPERTY(Config)
	TSoftClassPtr<UCommonActivatableWidget> ProgressWidgetClass;
	TWeakObjectPtr<UCommonSessionSubsystem> Sessions;
	TWeakObjectPtr<UUserWidget> SourceMenu;
	TWeakObjectPtr<UCommonActivatableWidget> ProgressWidget;
	FName InputToken;
	bool bRestoreEnabled = true;
	FDelegateHandle CreateHandle;
	FDelegateHandle JoinHandle;
};
