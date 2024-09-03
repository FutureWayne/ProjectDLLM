// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UButton;
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSION_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void MenuSetup(int32 InNumPublicConnections = 4, const FString& InMatchType = "FreeForAll", const FString& InPathToLobby = "/Game/ThirdPerson/Maps/Lobby?listen");

protected:
	virtual bool Initialize() override;

	virtual void NativeDestruct() override;

	/* Callbacks for custom delegate on the MultiplayerSessionSubsystem */
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);

	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);
	

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

	UFUNCTION()
	void OnHostButtonClicked();

	UFUNCTION()
	void OnJoinButtonClicked();

	void MenuTearDown();

	TObjectPtr<class UMultiplayerSessionSubsystem> MultiplayerSessionSubsystem;

	int32 NumPublicConnections{4};
	FString MatchType{"FreeForAll"};
	FString PathToLobby{"/Game/ThirdPerson/Maps/Lobby?listen"};
};
