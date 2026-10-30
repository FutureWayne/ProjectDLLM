// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Accolades/ArenaAccoladeDefinition.h"
#include "AsyncMixin.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "ArenaAccoladeHostWidget.generated.h"

class UObject;
class APlayerState;
class USoundBase;
class UUserWidget;
struct FDataRegistryAcquireResult;
struct FArenaNotificationMessage;

USTRUCT(BlueprintType)
struct FPendingAccoladeEntry
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly)
	FArenaAccoladeDefinitionRow Row; 

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString TargetPlayerName;

	/** Identity used for cancellation, even if the player leaves before this entry is displayed. */
	UPROPERTY()
	TWeakObjectPtr<APlayerState> TargetPlayer;

	UPROPERTY()
	TObjectPtr<UUserWidget> AllocatedWidget = nullptr;

	int32 SequenceID = 0;

	bool bFinishedLoading = false;
	bool bLoadFailed = false;

	void CancelDisplay();
};

/**
 * 
 */
UCLASS(BlueprintType)
class ARENA_API UArenaAccoladeHostWidget : public UCommonUserWidget, public FAsyncMixin
{
	GENERATED_BODY()

public:
	// The location tag (used to filter incoming messages to only display the appropriate accolades in a given location)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag LocationName;

	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~End of UUserWidget interface

	UFUNCTION(BlueprintImplementableEvent)
	void DestroyAccoladeWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent)
	UUserWidget* CreateAccoladeWidget(const FPendingAccoladeEntry& Entry);
private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FArenaAccoladeHostTestAccess;
#endif

	FGameplayMessageListenerHandle ListenerHandle;

	int32 NextDisplaySequenceID = 0;
	int32 AllocatedSequenceID = 0;
	uint32 LoadGeneration = 0;
	bool bAcceptingNotifications = false;

	FTimerHandle NextTimeToReconsiderHandle;

	// List of async pending load accolades (which might come in the wrong order due to the row read)
	UPROPERTY(Transient)
	TArray<FPendingAccoladeEntry> PendingAccoladeLoads;

	// List of pending accolades (due to one at a time display duration; the first one in the list is the current visible one)
	UPROPERTY(Transient)
	TArray<FPendingAccoladeEntry> PendingAccoladeDisplays;


	void OnNotificationMessage(FGameplayTag Channel, const FArenaNotificationMessage& Notification);
	void OnRegistryLoadCompleted(const FDataRegistryAcquireResult& AccoladeHandle, int32 SequenceID, uint32 Generation);
	void FailPendingAccolade(int32 SequenceID);
	void ResetAccolades();

	void ConsiderLoadedAccolades();
	void PopDisplayedAccolade();
	void ProcessLoadedAccolade(const FPendingAccoladeEntry& Entry);
	void DisplayNextAccolade();
};


