// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionSubsystem.h"
#include "OnlineSessionSettings.h"

void UMenu::MenuSetup(int32 InNumPublicConnections, const FString& InMatchType, const FString& InPathToLobby)
{
	NumPublicConnections = InNumPublicConnections;
	MatchType = InMatchType;
	PathToLobby = InPathToLobby;
	
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		MultiplayerSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
	}

	if (MultiplayerSessionSubsystem)
	{
		MultiplayerSessionSubsystem->MultiplayerOnCreateSessionCompleteDelegate.AddDynamic(this, &UMenu::OnCreateSession);
		MultiplayerSessionSubsystem->MultiplayerOnFindSessionsCompleteDelegate.AddUObject(this, &UMenu::OnFindSessions);
		MultiplayerSessionSubsystem->MultiplayerOnJoinSessionCompleteDelegate.AddUObject(this, &UMenu::OnJoinSession);
		MultiplayerSessionSubsystem->MultiplayerOnDestroySessionCompleteDelegate.AddDynamic(this, &UMenu::OnDestroySession);
		MultiplayerSessionSubsystem->MultiplayerOnStartSessionCompleteDelegate.AddDynamic(this, &UMenu::OnStartSession);
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UMenu::OnHostButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UMenu::OnJoinButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	
	Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Session Created Successfully"));
		}

		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to create game session!"));
		}

		HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful)
{
	if (MultiplayerSessionSubsystem == nullptr)
	{
		return;
	}

	for (auto SearchResult : SearchResults)
	{
		FString SettingValue;
		SearchResult.Session.SessionSettings.Get(FName("MatchType"), SettingValue);
		if (SettingValue == MatchType)
		{
			MultiplayerSessionSubsystem->JoinSession(SearchResult);
			return;
		}
	}

	if (!bWasSuccessful || SearchResults.Num() == 0)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}
	
	if (const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
	{
		const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
			{
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}

void UMenu::OnHostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	
	if (MultiplayerSessionSubsystem)
	{
		MultiplayerSessionSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::OnJoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	
	if (MultiplayerSessionSubsystem)
	{
		MultiplayerSessionSubsystem->FindSessions(10000);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
