// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonSessionSubsystem.h"
#include "AssetRegistry/AssetData.h"
#include "CommonUserTypes.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Misc/ConfigCacheIni.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineBeaconHost.h"
#include "OnlineSessionSettings.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "HAL/PlatformTime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CommonSessionSubsystem)

#if COMMONUSER_OSSV1
#include "Engine/World.h"
#include "OnlineSubsystemUtils.h"

FName SETTING_ONLINESUBSYSTEM_VERSION(TEXT("OSSv1"));
#else
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Online/OnlineServicesEngineUtils.h"

FName SETTING_ONLINESUBSYSTEM_VERSION(TEXT("OSSv2"));
using namespace UE::Online;
#endif // COMMONUSER_OSSV1


DECLARE_LOG_CATEGORY_EXTERN(LogCommonSession, Log, All);
DEFINE_LOG_CATEGORY(LogCommonSession);

#define LOCTEXT_NAMESPACE "CommonUser"

//////////////////////////////////////////////////////////////////////
//UCommonSession_SearchSessionRequest

void UCommonSession_SearchSessionRequest::NotifySearchFinished(bool bSucceeded, const FText& ErrorMessage)
{
	OnSearchFinished.Broadcast(bSucceeded, ErrorMessage);
	K2_OnSearchFinished.Broadcast(bSucceeded, ErrorMessage);
}


//////////////////////////////////////////////////////////////////////
//UCommonSession_SearchResult

#if COMMONUSER_OSSV1
FString UCommonSession_SearchResult::GetDescription() const
{
	return Result.GetSessionIdStr();
}

void UCommonSession_SearchResult::GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const
{
	bFoundValue = Result.Session.SessionSettings.Get<FString>(Key, /*out*/ Value);
}

void UCommonSession_SearchResult::GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const
{
	bFoundValue = Result.Session.SessionSettings.Get<int32>(Key, /*out*/ Value);
}

int32 UCommonSession_SearchResult::GetNumOpenPrivateConnections() const
{
	return Result.Session.NumOpenPrivateConnections;
}

int32 UCommonSession_SearchResult::GetNumOpenPublicConnections() const
{
	return Result.Session.NumOpenPublicConnections;
}

int32 UCommonSession_SearchResult::GetMaxPublicConnections() const
{
	return Result.Session.SessionSettings.NumPublicConnections;
}

int32 UCommonSession_SearchResult::GetPingInMs() const
{
	return Result.PingInMs;
}
#else
FString UCommonSession_SearchResult::GetDescription() const
{
	return ToLogString(Lobby->LobbyId);
}

void UCommonSession_SearchResult::GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const
{
	if (const FSchemaVariant* VariantValue = Lobby->Attributes.Find(Key))
	{
		bFoundValue = true;
		Value = VariantValue->GetString();
	}
	else
	{
		bFoundValue = false;
	}
}

void UCommonSession_SearchResult::GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const
{
	if (const FSchemaVariant* VariantValue = Lobby->Attributes.Find(Key))
	{
		bFoundValue = true;
		Value = (int32)VariantValue->GetInt64();
	}
	else
	{
		bFoundValue = false;
	}
}

int32 UCommonSession_SearchResult::GetNumOpenPrivateConnections() const
{
	// TODO:  Private connections
	return 0;
}

int32 UCommonSession_SearchResult::GetNumOpenPublicConnections() const
{
	return Lobby->MaxMembers - Lobby->Members.Num();
}

int32 UCommonSession_SearchResult::GetMaxPublicConnections() const
{
	return Lobby->MaxMembers;
}

int32 UCommonSession_SearchResult::GetPingInMs() const
{
	// TODO:  Not a property of lobbies.  Need to implement with sessions.
	return 0;
}
#endif //COMMONUSER_OSSV1


class FCommonOnlineSearchSettingsBase : public FGCObject
{
public:
	FCommonOnlineSearchSettingsBase(UCommonSession_SearchSessionRequest* InSearchRequest)
	{
		SearchRequest = InSearchRequest;
	}

	virtual ~FCommonOnlineSearchSettingsBase() {}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(SearchRequest);
	}

	virtual FString GetReferencerName() const override
	{
		static const FString NameString = TEXT("FCommonOnlineSearchSettings");
		return NameString;
	}

public:
	TObjectPtr<UCommonSession_SearchSessionRequest> SearchRequest = nullptr;
};

#if COMMONUSER_OSSV1
//////////////////////////////////////////////////////////////////////
// FCommonSession_OnlineSessionSettings

class FCommonSession_OnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FCommonSession_OnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4)
	{
		NumPublicConnections = MaxNumPlayers;
		if (NumPublicConnections < 0)
		{
			NumPublicConnections = 0;
		}
		NumPrivateConnections = 0;
		bIsLANMatch = bIsLAN;
		bShouldAdvertise = true;
		bAllowJoinInProgress = true;
		bAllowInvites = true;
		bUsesPresence = bIsPresence;
		bAllowJoinViaPresence = true;
		bAllowJoinViaPresenceFriendsOnly = false;
	}

	virtual ~FCommonSession_OnlineSessionSettings() {}
};

//////////////////////////////////////////////////////////////////////
// FCommonOnlineSearchSettingsOSSv1

class FCommonOnlineSearchSettingsOSSv1 : public FOnlineSessionSearch, public FCommonOnlineSearchSettingsBase
{
public:
	FCommonOnlineSearchSettingsOSSv1(UCommonSession_SearchSessionRequest* InSearchRequest)
		: FCommonOnlineSearchSettingsBase(InSearchRequest)
	{
		bIsLanQuery = (InSearchRequest->OnlineMode == ECommonSessionOnlineMode::LAN);
		MaxSearchResults = 10;
		TimeoutInSeconds = 30.0f;
		PingBucketSize = 50;

		QuerySettings.Set(SETTING_ONLINESUBSYSTEM_VERSION, true, EOnlineComparisonOp::Equals);

		if (InSearchRequest->bUseLobbies)
		{
			QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
		}
	}

	virtual ~FCommonOnlineSearchSettingsOSSv1() {}
};
#else

class FCommonOnlineSearchSettingsOSSv2 : public FCommonOnlineSearchSettingsBase
{
public:
	FCommonOnlineSearchSettingsOSSv2(UCommonSession_SearchSessionRequest* InSearchRequest)
		: FCommonOnlineSearchSettingsBase(InSearchRequest)
	{
		FindLobbyParams.MaxResults = 10;

		FindLobbyParams.Filters.Emplace(FFindLobbySearchFilter{ SETTING_ONLINESUBSYSTEM_VERSION, ESchemaAttributeComparisonOp::Equals, true });
	}
public:
	FFindLobbies::Params FindLobbyParams;
};

#endif // COMMONUSER_OSSV1

//////////////////////////////////////////////////////////////////////
// UCommonSession_HostSessionRequest

FString UCommonSession_HostSessionRequest::GetMapName() const
{
	FAssetData MapAssetData;
	if (UAssetManager::Get().GetPrimaryAssetData(MapID, /*out*/ MapAssetData))
	{
		return MapAssetData.PackageName.ToString();
	}
	else
	{
		return FString();
	}
}

FString UCommonSession_HostSessionRequest::ConstructTravelURL() const
{
	FString CombinedExtraArgs;

	if (OnlineMode == ECommonSessionOnlineMode::LAN)
	{
		CombinedExtraArgs += TEXT("?bIsLanMatch");
	}
	
	if (OnlineMode != ECommonSessionOnlineMode::Offline)
	{
		CombinedExtraArgs += TEXT("?listen");
	}

	for (const auto& KVP : ExtraArgs)
	{
		if (!KVP.Key.IsEmpty())
		{
			if (KVP.Value.IsEmpty())
			{
				CombinedExtraArgs += FString::Printf(TEXT("?%s"), *KVP.Key);
			}
			else
			{
				CombinedExtraArgs += FString::Printf(TEXT("?%s=%s"), *KVP.Key, *KVP.Value);
			}
		}
	}

	//bIsRecordingDemo ? TEXT("?DemoRec") : TEXT(""));

	return FString::Printf(TEXT("%s%s"),
		*GetMapName(),
		*CombinedExtraArgs);
}

bool UCommonSession_HostSessionRequest::ValidateAndLogErrors(FText& OutError) const
{
#if WITH_SERVER_CODE
	if (GetMapName().IsEmpty())
	{
		OutError = FText::Format(NSLOCTEXT("NetworkErrors", "InvalidMapFormat", "Can't find asset data for MapID {0}, hosting request failed."), FText::FromString(MapID.ToString()));
		return false;
	}

	return true;
#else
	// Client builds are only meant to connect to dedicated servers, they are missing the code to host a session by default
	// You can change this behavior in subclasses to handle something like a tutorial
	OutError = NSLOCTEXT("NetworkErrors", "ClientBuildCannotHost", "Client builds cannot host game sessions.");
	return false;
#endif
}

int32 UCommonSession_HostSessionRequest::GetMaxPlayers() const
{
	return MaxPlayerCount;
}

//////////////////////////////////////////////////////////////////////
// UCommonSessionSubsystem

void UCommonSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BindOnlineDelegates();
	GEngine->OnTravelFailure().AddUObject(this, &UCommonSessionSubsystem::TravelLocalSessionFailure);
	GEngine->OnNetworkFailure().AddUObject(this, &UCommonSessionSubsystem::NetworkSessionFailure);
	OperationTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickOperation), 0.25f);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCommonSessionSubsystem::HandlePostLoadMap);

	UGameInstance* GameInstance = GetGameInstance();
	bIsDedicatedServer = GameInstance->IsDedicatedServerInstance();
}

void UCommonSessionSubsystem::BindOnlineDelegates()
{
#if COMMONUSER_OSSV1
	BindOnlineDelegatesOSSv1();
#else
	BindOnlineDelegatesOSSv2();
#endif
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::BindOnlineDelegatesOSSv1()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete));
	SessionInterface->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete));
	SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnUpdateSessionComplete));
	SessionInterface->AddOnDestroySessionRequestedDelegate_Handle(FOnDestroySessionRequestedDelegate::CreateUObject(this, &ThisClass::OnDestroySessionRequested));

//	SessionInterface->AddOnMatchmakingCompleteDelegate_Handle(FOnMatchmakingCompleteDelegate::CreateUObject(this, &ThisClass::OnMatchmakingComplete));
//	SessionInterface->AddOnCancelMatchmakingCompleteDelegate_Handle(FOnCancelMatchmakingCompleteDelegate::CreateUObject(this, &ThisClass::OnCancelMatchmakingComplete));

	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete));
// 	SessionInterface->AddOnCancelFindSessionsCompleteDelegate_Handle(FOnCancelFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnCancelFindSessionsComplete));
// 	SessionInterface->AddOnPingSearchResultsCompleteDelegate_Handle(FOnPingSearchResultsCompleteDelegate::CreateUObject(this, &ThisClass::OnPingSearchResultsComplete));
	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete));

//	TWO_PARAM(OnSessionParticipantJoined, FName, const FUniqueNetId&);
//	THREE_PARAM(OnSessionParticipantLeft, FName, const FUniqueNetId&, EOnSessionParticipantLeftReason);
//	ONE_PARAM(OnQosDataRequested, FName);
//	TWO_PARAM(OnSessionCustomDataChanged, FName, const FOnlineSessionSettings&);
//	TWO_PARAM(OnSessionSettingsUpdated, FName, const FOnlineSessionSettings&);
//	THREE_PARAM(OnSessionParticipantSettingsUpdated, FName, const FUniqueNetId&, const FOnlineSessionSettings&);
//	FOUR_PARAM(OnSessionInviteReceived, const FUniqueNetId& /*UserId*/, const FUniqueNetId& /*FromId*/, const FString& /*AppId*/, const FOnlineSessionSearchResult& /*InviteResult*/);
//	THREE_PARAM(OnRegisterPlayersComplete, FName, const TArray< FUniqueNetIdRef >&, bool);
//	THREE_PARAM(OnUnregisterPlayersComplete, FName, const TArray< FUniqueNetIdRef >&, bool);

	SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleSessionUserInviteAccepted));
	SessionInterface->AddOnSessionFailureDelegate_Handle(FOnSessionFailureDelegate::CreateUObject(this, &ThisClass::HandleSessionFailure));
}

#else

void UCommonSessionSubsystem::BindOnlineDelegatesOSSv2()
{
	// TODO: Bind OSSv2 delegates when they are available
	// Note that most OSSv1 delegates above are implemented as completion delegates in OSSv2 and don't need to be subscribed to
	TSharedPtr<IOnlineServices> OnlineServices = GetServices(GetWorld());
	check(OnlineServices);
	ILobbiesPtr Lobbies = OnlineServices->GetLobbiesInterface();
	check(Lobbies);

	LobbyJoinRequestedHandle = Lobbies->OnUILobbyJoinRequested().Add(this, &UCommonSessionSubsystem::OnSessionJoinRequested);
}
#endif

void UCommonSessionSubsystem::Deinitialize()
{
#if COMMONUSER_OSSV1
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());

	if (OnlineSub)
	{
		// During shutdown this may not be valid
		const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
		if (SessionInterface)
		{
			SessionInterface->ClearOnSessionFailureDelegates(this);
			SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
			SessionInterface->ClearOnStartSessionCompleteDelegates(this);
			SessionInterface->ClearOnUpdateSessionCompleteDelegates(this);
			SessionInterface->ClearOnEndSessionCompleteDelegates(this);
			SessionInterface->ClearOnDestroySessionCompleteDelegates(this);
			SessionInterface->ClearOnDestroySessionRequestedDelegates(this);
			SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
			SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
			SessionInterface->ClearOnSessionUserInviteAcceptedDelegates(this);
		}
	}
#endif // COMMONUSER_OSSV1

	if (GEngine)
	{
		GEngine->OnTravelFailure().RemoveAll(this);
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	FTSTicker::GetCoreTicker().RemoveTicker(OperationTicker);
	ResetCleanupCallbacks();
#if COMMONUSER_OSSV1
	AbandonedSessionInterface.Reset();
#endif
	QueuedJoin = nullptr;
	SearchSettings.Reset();
	Super::Deinitialize();
}

bool UCommonSessionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass
	return ChildClasses.Num() == 0;
}

UCommonSession_HostSessionRequest* UCommonSessionSubsystem::CreateOnlineHostSessionRequest()
{
	/** Game-specific subsystems can override this or you can modify after creation */

	UCommonSession_HostSessionRequest* NewRequest = NewObject<UCommonSession_HostSessionRequest>(this);
	NewRequest->OnlineMode = ECommonSessionOnlineMode::Online;
	NewRequest->bUseLobbies = bUseLobbiesDefault;
	NewRequest->bUseLobbiesVoiceChat = bUseLobbiesVoiceChatDefault;

	// We enable presence by default in the primary session used for matchmaking. For online systems that care about presence, only the primary session should have presence enabled
	NewRequest->bUsePresence = !IsRunningDedicatedServer();

	return NewRequest;
}

UCommonSession_SearchSessionRequest* UCommonSessionSubsystem::CreateOnlineSearchSessionRequest()
{
	/** Game-specific subsystems can override this or you can modify after creation */

	UCommonSession_SearchSessionRequest* NewRequest = NewObject<UCommonSession_SearchSessionRequest>(this);
	NewRequest->OnlineMode = ECommonSessionOnlineMode::Online;

	NewRequest->bUseLobbies = bUseLobbiesDefault;

	return NewRequest;
}

void UCommonSessionSubsystem::HostSession(APlayerController* HostingPlayer, UCommonSession_HostSessionRequest* Request)
{
	if (IsSessionBusy())
	{
		UE_LOG(LogCommonSession, Warning, TEXT("Host rejected: another session operation is pending"));
		return;
	}
	BeginOperation(EOperation::Hosting);
	if (RejectQuarantinedOperation()) { return; }
	ULocalPlayer* LocalPlayer = HostingPlayer ? HostingPlayer->GetLocalPlayer() : nullptr;
	FText Error;
	if (!Request || (!LocalPlayer && !bIsDedicatedServer) || !GetWorld())
	{
		FailOperation(TEXT("InvalidRequest"), LOCTEXT("InvalidHostRequest", "A valid local player and hosting request are required."));
		return;
	}
	if (!Request->ValidateAndLogErrors(Error))
	{
		FailOperation(TEXT("InvalidRequest"), Error);
		return;
	}
	if (HasActiveSession())
	{
		FailOperation(TEXT("AlreadyInSession"), LOCTEXT("HostAlreadyInSession", "Leave the current session before hosting another game."));
		return;
	}
	bOfflineTravel = Request->OnlineMode == ECommonSessionOnlineMode::Offline;
	if (bOfflineTravel)
	{
		if (GetWorld()->GetNetMode() == NM_Client)
		{
			FailOperation(TEXT("CannotHostAsClient"), LOCTEXT("OfflineAsClient", "Leave the current game before hosting."));
			return;
		}
		bAwaitingTravel = true;
		TravelSourceWorld = GetWorld();
		if (!GetWorld()->ServerTravel(Request->ConstructTravelURL()))
		{
			FailOperation(TEXT("TravelFailed"), LOCTEXT("HostTravelRejected", "Could not open the requested map."));
		}
		return;
	}
	CreateOnlineSessionInternal(LocalPlayer, Request);
}

void UCommonSessionSubsystem::CreateOnlineSessionInternal(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request)
{
	CreateSessionResult = FOnlineResultInformation();
	PendingTravelURL = Request->ConstructTravelURL();

#if COMMONUSER_OSSV1
	CreateOnlineSessionInternalOSSv1(LocalPlayer, Request);
#else
	CreateOnlineSessionInternalOSSv2(LocalPlayer, Request);
#endif
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::CreateOnlineSessionInternalOSSv1(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request)
{
	const FName SessionName(NAME_GameSession);
	const int32 MaxPlayers = Request->GetMaxPlayers();

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (!OnlineSub)
	{
		FailOperation(TEXT("OnlineUnavailable"), LOCTEXT("HostNoOSS", "The online service is unavailable."));
		return;
	}

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		FailOperation(TEXT("OnlineUnavailable"), LOCTEXT("HostNoSessions", "The session service is unavailable."));
		return;
	}

	FUniqueNetIdPtr UserId;
	if (LocalPlayer)
	{
		UserId = LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
	}
	else if (bIsDedicatedServer && OnlineSub->GetIdentityInterface().IsValid())
	{
		UserId = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(DEDICATED_SERVER_USER_INDEX);
	}

	//@TODO: You can get here on some platforms while trying to do a LAN session, does that require a valid user id?
	if (UserId.IsValid() && UserId->IsValid())
	{
		FCommonSession_OnlineSessionSettings HostSettings(Request->OnlineMode == ECommonSessionOnlineMode::LAN, Request->bUsePresence, MaxPlayers);
		HostSettings.bUseLobbiesIfAvailable = Request->bUseLobbies;
		HostSettings.bUseLobbiesVoiceChatIfAvailable = Request->bUseLobbiesVoiceChat;
		HostSettings.Set(SETTING_GAMEMODE, Request->ModeNameForAdvertisement, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings.Set(SETTING_MAPNAME, Request->GetMapName(), EOnlineDataAdvertisementType::ViaOnlineService);
		//@TODO: HostSettings.Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
		HostSettings.Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings.Set(SETTING_SESSION_TEMPLATE_NAME, FString(TEXT("GameSession")), EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings.Set(SETTING_ONLINESUBSYSTEM_VERSION, true, EOnlineDataAdvertisementType::ViaOnlineService);

		bOwnsPendingSession = true;
		if (!Sessions->CreateSession(*UserId, SessionName, HostSettings) && ActiveOperation == EOperation::Hosting)
		{
			FailOperation(TEXT("CreateRejected"), LOCTEXT("CreateRejected", "The online service could not create a session."));
		}
	}
	else
	{
		OnCreateSessionComplete(SessionName, false);
	}
}

#else

void UCommonSessionSubsystem::CreateOnlineSessionInternalOSSv2(ULocalPlayer* LocalPlayer, UCommonSession_HostSessionRequest* Request)
{
	// Only lobbies are supported for now
	if (!ensureMsgf(Request->bUseLobbies, TEXT("Only Lobbies are supported in this release")))
	{
		Request->bUseLobbies = true;
	}

	const FName SessionName(NAME_GameSession);
	const int32 MaxPlayers = Request->GetMaxPlayers();

	IOnlineServicesPtr OnlineServices = GetServices(GetWorld());
	check(OnlineServices);
	ILobbiesPtr Lobbies = OnlineServices->GetLobbiesInterface();
	check(Lobbies);
	FCreateLobby::Params CreateParams;

	if (LocalPlayer)
	{
		CreateParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
	}
	else if (bIsDedicatedServer)
	{
		// TODO what should this do for v2?
	}

	CreateParams.LocalName = SessionName;
	CreateParams.SchemaId = FSchemaId(TEXT("GameLobby")); // TODO: make a parameter
	CreateParams.bPresenceEnabled = Request->bUsePresence;
	CreateParams.MaxMembers = MaxPlayers;
	CreateParams.JoinPolicy = ELobbyJoinPolicy::PublicAdvertised; // TODO: Check parameters

	CreateParams.Attributes.Emplace(SETTING_GAMEMODE, Request->ModeNameForAdvertisement);
	CreateParams.Attributes.Emplace(SETTING_MAPNAME, Request->GetMapName());
	//@TODO: CreateParams.Attributes.Emplace(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"));
	CreateParams.Attributes.Emplace(SETTING_MATCHING_TIMEOUT, 120.0f);
	CreateParams.Attributes.Emplace(SETTING_SESSION_TEMPLATE_NAME, FString(TEXT("GameSession")));
	CreateParams.Attributes.Emplace(SETTING_ONLINESUBSYSTEM_VERSION, true);

	CreateParams.UserAttributes.Emplace(SETTING_GAMEMODE, FString(TEXT("GameSession")));

	// TODO: Add splitscreen players

	FString ModeName = Request->ModeNameForAdvertisement;
	FString MapName = Request->GetMapName();

	Lobbies->CreateLobby(MoveTemp(CreateParams)).OnComplete(this, [this, SessionName, ModeName, MapName](const TOnlineResult<FCreateLobby>& CreateResult)
	{
		OnCreateSessionComplete(SessionName, CreateResult.IsOk());
		NotifySessionInformationUpdated(ECommonSessionInformationState::InGame, ModeName, MapName);
	});
}

#endif

void UCommonSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession) { return; }
	UE_LOG(LogCommonSession, Log, TEXT("OnCreateSessionComplete(SessionName: %s, bWasSuccessful: %d)"), *SessionName.ToString(), bWasSuccessful);
	if (bWantToDestroyPendingSession)
	{
#if COMMONUSER_OSSV1
		CleanUpSessionsOSSv1();
#endif
		return;
	}
	if (ActiveOperation != EOperation::Hosting) { return; }


#if COMMONUSER_OSSV1 // OSSv2 joins splitscreen players as part of the create call
	// Add the splitscreen player if one exists
#if 0 //@TODO:
	if (bWasSuccessful && LocalPlayers.Num() > 1)
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
		if (Sessions.IsValid() && LocalPlayers[1]->GetPreferredUniqueNetId().IsValid())
		{
			Sessions->RegisterLocalPlayer(*LocalPlayers[1]->GetPreferredUniqueNetId(), NAME_GameSession,
				FOnRegisterLocalPlayerCompleteDelegate::CreateUObject(this, &ThisClass::OnRegisterLocalPlayerComplete_CreateSession));
		}
	}
	else
#endif
#endif
	{
		// We either failed or there is only a single local user
		FinishSessionCreation(bWasSuccessful);
	}
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::OnRegisterLocalPlayerComplete_CreateSession(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishSessionCreation(Result == EOnJoinSessionCompleteResult::Success);
}

void UCommonSessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogCommonSession, Log, TEXT("OnStartSessionComplete(SessionName: %s, bWasSuccessful: %d)"), *SessionName.ToString(), bWasSuccessful);

	if (bWantToDestroyPendingSession)
	{
		CleanUpSessionsOSSv1();
	}
}
#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::FinishSessionCreation(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		FailOperation(TEXT("CreateFailed"), CreateSessionResult.ErrorText.IsEmpty() ? LOCTEXT("CreateFailed", "Failed to create the session.") : CreateSessionResult.ErrorText);
		return;
	}
	if (bUseBeacons) { CreateHostReservationBeacon(); }
	bAwaitingTravel = true;
	TravelSourceWorld = GetWorld();
	if (!GetWorld() || !GetWorld()->ServerTravel(PendingTravelURL))
	{
		FailOperation(TEXT("TravelFailed"), LOCTEXT("HostTravelRejected", "Could not open the requested map."));
	}
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogCommonSession, Log, TEXT("OnUpdateSessionComplete(SessionName: %s, bWasSuccessful: %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
}

void UCommonSessionSubsystem::OnEndSessionComplete(FName SessionName, bool bWasSuccessful, uint64 CleanupId)
{
	if (SessionName != NAME_GameSession || !bWantToDestroyPendingSession || bCleanupQuarantined || CleanupId != CleanupGeneration) { return; }
	if (!bWasSuccessful) { FinishCleanup(false); return; }
	CleanUpSessionsOSSv1();
}

void UCommonSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful, uint64 CleanupId)
{
	if (SessionName == NAME_GameSession && bWantToDestroyPendingSession && !bCleanupQuarantined && CleanupId == CleanupGeneration) { FinishCleanup(bWasSuccessful); }
}

void UCommonSessionSubsystem::OnDestroySessionRequested(int32 LocalUserNum, FName SessionName)
{
	FPlatformUserId PlatformUserId = IPlatformInputDeviceMapper::Get().GetPlatformUserForUserIndex(LocalUserNum);

	NotifyDestroySessionRequested(PlatformUserId, SessionName);
}
#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::FindSessions(APlayerController* SearchingPlayer, UCommonSession_SearchSessionRequest* Request)
{
	if (Request == nullptr)
	{
		UE_LOG(LogCommonSession, Error, TEXT("FindSessions passed a null request"));
		return;
	}

#if COMMONUSER_OSSV1
	FindSessionsInternal(SearchingPlayer, MakeShared<FCommonOnlineSearchSettingsOSSv1>(Request));
#else
	FindSessionsInternal(SearchingPlayer, MakeShared<FCommonOnlineSearchSettingsOSSv2>(Request));
#endif // COMMONUSER_OSSV1
}

void UCommonSessionSubsystem::FindSessionsInternal(APlayerController* SearchingPlayer, const TSharedRef<FCommonOnlineSearchSettings>& InSearchSettings)
{
	if (bCleanupQuarantined)
	{
		InSearchSettings->SearchRequest->NotifySearchFinished(false, LOCTEXT("CleanupQuarantinedSearch", "Steam is still leaving the previous session. Please wait, or restart the game if Steam does not recover."));
		return;
	}
	if (SearchSettings.IsValid())
	{
		//@TODO: This is a poor user experience for the API user, we should let the additional search piggyback and
		// just give it the same results as the currently pending one
		// (or enqueue the request and service it when the previous one finishes or fails)
		UE_LOG(LogCommonSession, Warning, TEXT("A previous FindSessions call is still in progress, rejecting the new request"));
		InSearchSettings->SearchRequest->NotifySearchFinished(false, LOCTEXT("Error_FindSessionAlreadyInProgress", "Session search already in progress"));
		return;
	}

	ULocalPlayer* LocalPlayer = (SearchingPlayer != nullptr) ? SearchingPlayer->GetLocalPlayer() : nullptr;
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogCommonSession, Error, TEXT("SearchingPlayer is invalid"));
		InSearchSettings->SearchRequest->NotifySearchFinished(false, LOCTEXT("Error_FindSessionBadPlayer", "Session search was not provided a local player"));
		return;
	}

	SearchSettings = InSearchSettings;
#if COMMONUSER_OSSV1
	FindSessionsInternalOSSv1(LocalPlayer);
#else
	FindSessionsInternalOSSv2(LocalPlayer);
#endif
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::FindSessionsInternalOSSv1(ULocalPlayer* LocalPlayer)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
	const FUniqueNetIdPtr UserId = LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
	if (!Sessions.IsValid() || !UserId.IsValid() || !UserId->IsValid())
	{
		TSharedPtr<FCommonOnlineSearchSettings> CompletedSearch = MoveTemp(SearchSettings);
		CompletedSearch->SearchRequest->NotifySearchFinished(false, LOCTEXT("SearchUnavailable", "Sign in to Steam before searching for games."));
		return;
	}
	SearchSettings->QuerySettings.Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineComparisonOp::Equals);
	const TSharedRef<FCommonOnlineSearchSettingsOSSv1> StartedSearch = SearchSettings.ToSharedRef();
	if (!Sessions->FindSessions(*UserId, StartedSearch) && SearchSettings == StartedSearch)
	{
		StartedSearch->SearchState = EOnlineAsyncTaskState::Failed;
		OnFindSessionsComplete(false);
	}
}

#else

void UCommonSessionSubsystem::FindSessionsInternalOSSv2(ULocalPlayer* LocalPlayer)
{
	IOnlineServicesPtr OnlineServices = GetServices(GetWorld());
	check(OnlineServices);
	ILobbiesPtr Lobbies = OnlineServices->GetLobbiesInterface();
	check(Lobbies);

	FFindLobbies::Params FindLobbyParams = StaticCastSharedPtr<FCommonOnlineSearchSettingsOSSv2>(SearchSettings)->FindLobbyParams;
	FindLobbyParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();

	Lobbies->FindLobbies(MoveTemp(FindLobbyParams)).OnComplete(this, [this, LocalSearchSettings = SearchSettings](const TOnlineResult<FFindLobbies>& FindResult)
	{
		if (LocalSearchSettings != SearchSettings)
		{
			// This was an abandoned search, ignore
			return;
		}
		const bool bWasSuccessful = FindResult.IsOk();
		UE_LOG(LogCommonSession, Log, TEXT("FindLobbies(bWasSuccessful: %s)"), *LexToString(bWasSuccessful));
		check(SearchSettings.IsValid());
		if (bWasSuccessful)
		{
			const FFindLobbies::Result& FindResults = FindResult.GetOkValue();
			SearchSettings->SearchRequest->Results.Reset(FindResults.Lobbies.Num());

			for (const TSharedRef<const FLobby>& Lobby : FindResults.Lobbies)
			{
				if (!Lobby->OwnerAccountId.IsValid())
				{
					UE_LOG(LogCommonSession, Verbose, TEXT("\tIgnoring Lobby with no owner (LobbyId: %s)"),
						*ToLogString(Lobby->LobbyId));
				}
				else if (Lobby->Members.Num() == 0)
				{
					UE_LOG(LogCommonSession, Verbose, TEXT("\tIgnoring Lobby with no members (UserId: %s)"),
						*ToLogString(Lobby->OwnerAccountId));
				}
				else
				{
					UCommonSession_SearchResult* Entry = NewObject<UCommonSession_SearchResult>(SearchSettings->SearchRequest);
					Entry->Lobby = Lobby;
					SearchSettings->SearchRequest->Results.Add(Entry);

					UE_LOG(LogCommonSession, Log, TEXT("\tFound lobby (UserId: %s, NumOpenConns: %d)"),
						*ToLogString(Lobby->OwnerAccountId), Lobby->MaxMembers - Lobby->Members.Num());
				}
			}
		}
		else
		{
			SearchSettings->SearchRequest->Results.Empty();
		}

		const FText ResultText = bWasSuccessful ? FText() : FindResult.GetErrorValue().GetText();

		SearchSettings->SearchRequest->NotifySearchFinished(bWasSuccessful, ResultText);
		SearchSettings.Reset();
	});
}
#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::QuickPlaySession(APlayerController* JoiningOrHostingPlayer, UCommonSession_HostSessionRequest* HostRequest)
{
	UE_LOG(LogCommonSession, Log, TEXT("QuickPlay Requested"));

	if (IsSessionBusy()) { return; }
	BeginOperation(EOperation::QuickPlay);
	if (RejectQuarantinedOperation()) { return; }
	if (HostRequest == nullptr)
	{
		FailOperation(TEXT("InvalidRequest"), LOCTEXT("InvalidQuickPlay", "A valid hosting request is required."));
		return;
	}

	TStrongObjectPtr<UCommonSession_HostSessionRequest> HostRequestPtr = TStrongObjectPtr<UCommonSession_HostSessionRequest>(HostRequest);
	TWeakObjectPtr<APlayerController> JoiningOrHostingPlayerPtr = TWeakObjectPtr<APlayerController>(JoiningOrHostingPlayer);

	UCommonSession_SearchSessionRequest* QuickPlayRequest = CreateOnlineSearchSessionRequest();
	QuickPlaySearchRequest = QuickPlayRequest;
	QuickPlayRequest->OnSearchFinished.AddUObject(this, &UCommonSessionSubsystem::HandleQuickPlaySearchFinished, JoiningOrHostingPlayerPtr, HostRequestPtr, QuickPlayRequest);

	// We enable presence by default on the primary session used for matchmaking. For online systems that care about presence, only the primary session should have presence enabled

	HostRequestPtr->bUseLobbies = bUseLobbiesDefault;
	HostRequestPtr->bUseLobbiesVoiceChat = bUseLobbiesVoiceChatDefault;
	HostRequestPtr->bUsePresence = true;
	QuickPlayRequest->bUseLobbies = bUseLobbiesDefault;

	NotifySessionInformationUpdated(ECommonSessionInformationState::Matchmaking);
	FindSessionsInternal(JoiningOrHostingPlayer, CreateQuickPlaySearchSettings(HostRequest, QuickPlayRequest));
}

TSharedRef<FCommonOnlineSearchSettings> UCommonSessionSubsystem::CreateQuickPlaySearchSettings(UCommonSession_HostSessionRequest* HostRequest, UCommonSession_SearchSessionRequest* SearchRequest)
{
#if COMMONUSER_OSSV1
	return CreateQuickPlaySearchSettingsOSSv1(HostRequest, SearchRequest);
#else
	return CreateQuickPlaySearchSettingsOSSv2(HostRequest, SearchRequest);
#endif // COMMONUSER_OSSV1
}

#if COMMONUSER_OSSV1
TSharedRef<FCommonOnlineSearchSettings> UCommonSessionSubsystem::CreateQuickPlaySearchSettingsOSSv1(UCommonSession_HostSessionRequest* HostRequest, UCommonSession_SearchSessionRequest* SearchRequest)
{
	TSharedRef<FCommonOnlineSearchSettingsOSSv1> QuickPlaySearch = MakeShared<FCommonOnlineSearchSettingsOSSv1>(SearchRequest);

	/** By default quick play does not want to include the map or game mode, games can fill this in as desired
	if (!HostRequest->ModeNameForAdvertisement.IsEmpty())
	{
		QuickPlaySearch->QuerySettings.Set(SETTING_GAMEMODE, HostRequest->ModeNameForAdvertisement, EOnlineComparisonOp::Equals);
	}

	if (!HostRequest->GetMapName().IsEmpty())
	{
		QuickPlaySearch->QuerySettings.Set(SETTING_MAPNAME, HostRequest->GetMapName(), EOnlineComparisonOp::Equals);
	} 
	*/

	// QuickPlaySearch->QuerySettings.Set(SEARCH_DEDICATED_ONLY, true, EOnlineComparisonOp::Equals);
	return QuickPlaySearch;
}

#else

TSharedRef<FCommonOnlineSearchSettings> UCommonSessionSubsystem::CreateQuickPlaySearchSettingsOSSv2(UCommonSession_HostSessionRequest* HostRequest, UCommonSession_SearchSessionRequest* SearchRequest)
{
	TSharedRef<FCommonOnlineSearchSettingsOSSv2> QuickPlaySearch = MakeShared<FCommonOnlineSearchSettingsOSSv2>(SearchRequest);

	/** By default quick play does not want to include the map or game mode, games can fill this in as desired
	if (!HostRequest->ModeNameForAdvertisement.IsEmpty())
	{
		QuickPlaySearch->FindLobbyParams.Filters.Emplace(FFindLobbySearchFilter{SETTING_GAMEMODE, ESchemaAttributeComparisonOp::Equals, HostRequest->ModeNameForAdvertisement});
	}
	if (!HostRequest->GetMapName().IsEmpty())
	{
		QuickPlaySearch->FindLobbyParams.Filters.Emplace(FFindLobbySearchFilter{SETTING_MAPNAME, ESchemaAttributeComparisonOp::Equals, HostRequest->GetMapName()});
	}
	*/

	return QuickPlaySearch;
}

#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::HandleQuickPlaySearchFinished(bool bSucceeded, const FText& ErrorMessage, TWeakObjectPtr<APlayerController> JoiningOrHostingPlayer, TStrongObjectPtr<UCommonSession_HostSessionRequest> HostRequest, UCommonSession_SearchSessionRequest* SearchRequest)
{
	if (ActiveOperation != EOperation::QuickPlay || SearchRequest != QuickPlaySearchRequest) { return; }
	if (!bSucceeded || !JoiningOrHostingPlayer.IsValid())
	{
		FailOperation(TEXT("QuickPlaySearchFailed"), ErrorMessage.IsEmpty() ? LOCTEXT("QuickPlayFailed", "Could not find a game. Please try again.") : ErrorMessage);
		return;
	}
	ActiveOperation = EOperation::None;
	for (UCommonSession_SearchResult* Result : SearchRequest->Results)
	{
		if (Result && Result->GetNumOpenPublicConnections() > 0)
		{
			JoinSession(JoiningOrHostingPlayer.Get(), Result);
			return;
		}
	}
	HostSession(JoiningOrHostingPlayer.Get(), HostRequest.Get());
}

void UCommonSessionSubsystem::CleanUpSessions()
{
	if (bCleanupQuarantined) { return; }
	bRecoverAfterCleanup = false;
	QueuedJoin = nullptr;
	const bool bAlreadyCleaning = bWantToDestroyPendingSession;
	BeginCleanup();
	if (ActiveOperation != EOperation::None)
	{
		FOnlineResultInformation Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Cancelled");
		Result.ErrorText = LOCTEXT("SessionCancelled", "The session request was cancelled.");
		CancelActiveSearch(Result.ErrorText);
		CompleteOperation(Result);
	}
	if (bAlreadyCleaning) { return; }
	if (bUseBeacons) { DestroyHostReservationBeacon(); }
	NotifySessionInformationUpdated(ECommonSessionInformationState::OutOfGame);
#if COMMONUSER_OSSV1
	CleanUpSessionsOSSv1();
#else
	CleanUpSessionsOSSv2();
#endif
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::CleanUpSessionsOSSv1()
{
	const IOnlineSessionPtr Sessions = CleanupSessionInterface.IsValid() ? CleanupSessionInterface : Online::GetSessionInterface(GetWorld());
	if (!Sessions.IsValid()) { FinishCleanup(!bOwnsPendingSession); return; }
	const EOnlineSessionState::Type State = Sessions->GetSessionState(NAME_GameSession);
	if (!Sessions->GetNamedSession(NAME_GameSession)) { FinishCleanup(true); return; }
	// Steam lobby joins may own a named session whose state is still NoSession.
	if (State == EOnlineSessionState::NoSession) { return; }
	if (State == EOnlineSessionState::Creating || State == EOnlineSessionState::Starting ||
		State == EOnlineSessionState::Ending || State == EOnlineSessionState::Destroying) { return; }
	if (State == EOnlineSessionState::InProgress)
	{
		if (!Sessions->EndSession(NAME_GameSession) && bWantToDestroyPendingSession &&
			Sessions->GetSessionState(NAME_GameSession) == State) { FinishCleanup(false); }
	}
	else if (!Sessions->DestroySession(NAME_GameSession) && bWantToDestroyPendingSession)
	{
		FinishCleanup(false);
	}
}

#else
void UCommonSessionSubsystem::CleanUpSessionsOSSv2()
{
	IOnlineServicesPtr OnlineServices = GetServices(GetWorld());
	check(OnlineServices);
	ILobbiesPtr Lobbies = OnlineServices->GetLobbiesInterface();
	check(Lobbies);

	FAccountId LocalPlayerId = GetAccountId(GetGameInstance()->GetFirstLocalPlayerController());
	FLobbyId LobbyId = GetLobbyId(NAME_GameSession);

	if (!LocalPlayerId.IsValid() || !LobbyId.IsValid())
	{
		return;
	}
	// TODO:  Include all local players leave the lobby
	Lobbies->LeaveLobby({LocalPlayerId, LobbyId});
}

#endif // COMMONUSER_OSSV1

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogCommonSession, Log, TEXT("OnFindSessionsComplete(bWasSuccessful: %s)"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (!SearchSettings.IsValid())
	{
		// This could get called twice for failed session searches, or for a search requested by a different system
		return;
	}

	const TSharedPtr<FCommonOnlineSearchSettingsOSSv1> CompletedSearch = SearchSettings;
	FCommonOnlineSearchSettingsOSSv1& SearchSettingsV1 = *CompletedSearch;
	if (SearchSettingsV1.SearchState == EOnlineAsyncTaskState::InProgress || SearchSettingsV1.SearchState == EOnlineAsyncTaskState::NotStarted)
	{
		UE_LOG(LogCommonSession, Verbose, TEXT("Ignoring completion while the active search has not finished"));
		return;
	}

	// OSSv1 broadcasts no request identity. Cancelled Steam queries can still broadcast;
	// only the active search object's own terminal state is authoritative.
	bWasSuccessful = SearchSettingsV1.SearchState == EOnlineAsyncTaskState::Done;
	if (!ensure(SearchSettingsV1.SearchRequest))
	{
		UE_LOG(LogCommonSession, Error, TEXT("OnFindSessionsComplete called with invalid search request object!"));
		return;
	}

	if (bWasSuccessful)
	{
		SearchSettingsV1.SearchRequest->Results.Reset(SearchSettingsV1.SearchResults.Num());

		for (const FOnlineSessionSearchResult& Result : SearchSettingsV1.SearchResults)
		{
			if (!Result.IsValid()) { continue; }

			UCommonSession_SearchResult* Entry = NewObject<UCommonSession_SearchResult>(SearchSettingsV1.SearchRequest);
			Entry->Result = Result;
			SearchSettingsV1.SearchRequest->Results.Add(Entry);

			FString SessionId = TEXT("Unknown");
			if (Result.Session.SessionInfo.IsValid())
			{
				SessionId = Result.Session.SessionInfo->GetSessionId().ToString();
			}
			FString OwningUserId = TEXT("Unknown");
			if (Result.Session.OwningUserId.IsValid())
			{
				OwningUserId = Result.Session.OwningUserId->ToString();
			}

			UE_LOG(LogCommonSession, Log, TEXT("\tFound session (SessionId: %s, UserId: %s, UserName: %s, NumOpenPrivConns: %d, NumOpenPubConns: %d, Ping: %d ms"),
				*SessionId,
				*OwningUserId,
				*Result.Session.OwningUserName,
				Result.Session.NumOpenPrivateConnections,
				Result.Session.NumOpenPublicConnections,
				Result.PingInMs
				);
		}
	}
	else
	{
		SearchSettingsV1.SearchRequest->Results.Empty();
	}

	
	SearchSettings.Reset();
	SearchSettingsV1.SearchRequest->NotifySearchFinished(bWasSuccessful, bWasSuccessful ? FText() : LOCTEXT("Error_FindSessionV1Failed", "Find session failed"));
}
#endif // COMMONUSER_OSSV1


void UCommonSessionSubsystem::JoinSession(APlayerController* Player, UCommonSession_SearchResult* Request)
{
	if (IsSessionBusy()) { return; }
	BeginOperation(EOperation::Joining);
	if (RejectQuarantinedOperation()) { return; }
	JoiningPlayer = Player;
	ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	if (!Request || !LocalPlayer)
	{
		FailOperation(TEXT("InvalidRequest"), LOCTEXT("InvalidJoin", "A valid local player and session are required."));
		return;
	}
#if COMMONUSER_OSSV1
	if (!Request->Result.IsValid() || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		FailOperation(TEXT("InvalidSession"), LOCTEXT("InvalidJoinSession", "The session is no longer valid, or Steam is not signed in."));
		return;
	}
#endif
	if (HasActiveSession())
	{
		QueuedJoin = Request;
		BeginCleanup();
#if COMMONUSER_OSSV1
		CleanUpSessionsOSSv1();
#endif
		return;
	}
	JoinSessionInternal(LocalPlayer, Request);
}

void UCommonSessionSubsystem::JoinSessionInternal(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request)
{
#if COMMONUSER_OSSV1
	JoinSessionInternalOSSv1(LocalPlayer, Request);
#else
	JoinSessionInternalOSSv2(LocalPlayer, Request);
#endif // COMMONUSER_OSSV1
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::JoinSessionInternalOSSv1(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
	const FUniqueNetIdPtr UserId = LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
	if (!Sessions.IsValid() || !UserId.IsValid() || !UserId->IsValid())
	{
		FailOperation(TEXT("OnlineUnavailable"), LOCTEXT("JoinNoOSS", "Sign in to Steam before joining a game."));
		return;
	}
	Request->Result.Session.SessionSettings.bUsesPresence = true;
	Request->Result.Session.SessionSettings.bUseLobbiesIfAvailable = true;
	bOwnsPendingSession = true;
	if (!Sessions->JoinSession(*UserId, NAME_GameSession, Request->Result) && ActiveOperation == EOperation::Joining)
	{
		FailOperation(TEXT("JoinRejected"), LOCTEXT("JoinRejected", "The online service could not join the session."));
	}
}

void UCommonSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionName != NAME_GameSession) { return; }
	if (bWantToDestroyPendingSession && !QueuedJoin) { CleanUpSessionsOSSv1(); return; }
	if (ActiveOperation == EOperation::Joining && !bAwaitingTravel) { FinishJoinSession(Result); }
}

void UCommonSessionSubsystem::OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishJoinSession(Result);
}

void UCommonSessionSubsystem::ConnectToHostReservationBeacon()
{
	UWorld* const World = GetWorld();
	check(World);
	ReservationBeaconClient = World->SpawnActor<APartyBeaconClient>(APartyBeaconClient::StaticClass());
	check(ReservationBeaconClient.IsValid());

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(World);
	check(OnlineSub);
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	check(Sessions);
	FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
	check(Session);
	FString SessionIdStr = Session->GetSessionIdStr();

	FString ConnectInfo;
	Sessions->GetResolvedConnectString(NAME_GameSession, ConnectInfo, NAME_BeaconPort);

	IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	check(Identity);
	FUniqueNetIdWrapper DefaultNetId = Identity->GetUniquePlayerId(0);
	check(DefaultNetId.IsValid());

	FPlayerReservation PlayerReservation;
	PlayerReservation.UniqueId = *DefaultNetId;
	PlayerReservation.Platform = OnlineSub->GetLocalPlatformName();

	ReservationBeaconClient->OnHostConnectionFailure().BindWeakLambda(this, [this]()
		{
			// We only want to react to failure calls while the connection is active, not when it closes
			if(ReservationBeaconClient->GetNetDriver())
			{
				FOnlineResultInformation JoinSessionResult;
				JoinSessionResult.bWasSuccessful = false;
				JoinSessionResult.ErrorId = TEXT("UnknownError");

				NotifyJoinSessionComplete(JoinSessionResult);
				NotifySessionInformationUpdated(ECommonSessionInformationState::OutOfGame);

				CleanUpSessions();
			}
		});

	ReservationBeaconClient->OnReservationRequestComplete().BindWeakLambda(this, [this](EPartyReservationResult::Type ReservationResponse)
		{
			if (ReservationResponse == EPartyReservationResult::ReservationAccepted)
			{
				FOnlineResultInformation JoinSessionResult;
				JoinSessionResult.bWasSuccessful = true;
				InternalTravelToSession(NAME_GameSession);
			}
			else
			{
				FOnlineResultInformation JoinSessionResult;
				JoinSessionResult.bWasSuccessful = false;
				JoinSessionResult.ErrorId = TEXT("UnknownError");

				NotifyJoinSessionComplete(JoinSessionResult);
				NotifySessionInformationUpdated(ECommonSessionInformationState::OutOfGame);

				CleanUpSessions();
			}
		});

	ReservationBeaconClient->RequestReservation(ConnectInfo, SessionIdStr, *DefaultNetId, { PlayerReservation });
}

void UCommonSessionSubsystem::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		if (bUseBeacons) { ConnectToHostReservationBeacon(); }
		else { InternalTravelToSession(NAME_GameSession); }
		return;
	}
	const FText Error = Result == EOnJoinSessionCompleteResult::SessionIsFull ? LOCTEXT("SessionIsFull", "Game is full.") :
		Result == EOnJoinSessionCompleteResult::SessionDoesNotExist ? LOCTEXT("SessionDoesNotExist", "Game no longer exists.") : LOCTEXT("JoinFailed", "Failed to join the game.");
	FailOperation(LexToString(Result), Error);
}

#else

void UCommonSessionSubsystem::JoinSessionInternalOSSv2(ULocalPlayer* LocalPlayer, UCommonSession_SearchResult* Request)
{
	const FName SessionName(NAME_GameSession);
	IOnlineServicesPtr OnlineServices = GetServices(GetWorld());
	check(OnlineServices);
	ILobbiesPtr Lobbies = OnlineServices->GetLobbiesInterface();
	check(Lobbies);

	FJoinLobby::Params JoinParams;
	JoinParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
	JoinParams.LocalName = SessionName;
	JoinParams.LobbyId = Request->Lobby->LobbyId;
	JoinParams.bPresenceEnabled = true;

	// Add any splitscreen players if they exist //@TODO: See UCommonSessionSubsystem::OnJoinSessionComplete

	Lobbies->JoinLobby(MoveTemp(JoinParams)).OnComplete(this, [this, SessionName](const TOnlineResult<FJoinLobby>& JoinResult)
	{
		if (JoinResult.IsOk())
		{
			InternalTravelToSession(SessionName);
		}
		else
		{
			//@TODO: Error handling
			UE_LOG(LogCommonSession, Error, TEXT("JoinLobby Failed with Result: %s"), *ToLogString(JoinResult.GetErrorValue()));
		}
	});
}

void UCommonSessionSubsystem::OnSessionJoinRequested(const UE::Online::FUILobbyJoinRequested& EventParams)
{
	TSharedPtr<IOnlineServices> OnlineServices = GetServices(GetWorld());
	check(OnlineServices);
	IAuthPtr Auth = OnlineServices->GetAuthInterface();
	check(Auth);
	TOnlineResult<FAuthGetLocalOnlineUserByOnlineAccountId> Account = Auth->GetLocalOnlineUserByOnlineAccountId({ EventParams.LocalAccountId });
	if (Account.IsOk())
	{
		FPlatformUserId PlatformUserId = Account.GetOkValue().AccountInfo->PlatformUserId;
		UCommonSession_SearchResult* RequestedSession = nullptr;
		FOnlineResultInformation ResultInfo;
		if (EventParams.Result.IsOk())
		{
			RequestedSession = NewObject<UCommonSession_SearchResult>(this);
			RequestedSession->Lobby = EventParams.Result.GetOkValue();
		}
		else
		{
			ResultInfo.FromOnlineError(EventParams.Result.GetErrorValue());
		}
		NotifyUserRequestedSession(PlatformUserId, RequestedSession, ResultInfo);
	}
	else
	{
		UE_LOG(LogCommonSession, Error, TEXT("OnJoinLobbyRequested: Failed to get account by local user id %s"), *UE::Online::ToLogString(EventParams.LocalAccountId));
	}
}

UE::Online::FAccountId UCommonSessionSubsystem::GetAccountId(APlayerController* PlayerController) const
{
	if (const ULocalPlayer* const LocalPlayer = PlayerController->GetLocalPlayer())
	{
		FUniqueNetIdRepl LocalPlayerIdRepl = LocalPlayer->GetPreferredUniqueNetId();
		if (LocalPlayerIdRepl.IsValid())
		{
			return LocalPlayerIdRepl.GetV2();
		}
	}
	return FAccountId();
}

UE::Online::FLobbyId UCommonSessionSubsystem::GetLobbyId(const FName SessionName) const
{
	FAccountId LocalUserId = GetAccountId(GetGameInstance()->GetFirstLocalPlayerController());
	if (LocalUserId.IsValid())
	{
		IOnlineServicesPtr OnlineServices = GetServices(GetWorld());
		check(OnlineServices);
		ILobbiesPtr Lobbies = OnlineServices->GetLobbiesInterface();
		check(Lobbies);
		TOnlineResult<FGetJoinedLobbies> JoinedLobbies = Lobbies->GetJoinedLobbies({ LocalUserId });
		if (JoinedLobbies.IsOk())
		{
			for (const TSharedRef<const FLobby>& Lobby : JoinedLobbies.GetOkValue().Lobbies)
			{
				if (Lobby->LocalName == SessionName)
				{
					return Lobby->LobbyId;
				}
			}
		}
	}
	return FLobbyId();
}

#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::InternalTravelToSession(const FName SessionName)
{
	APlayerController* Player = JoiningPlayer.Get();
	if (!Player)
	{
		FailOperation(TEXT("InvalidPlayer"), LOCTEXT("TravelNoPlayer", "The local player is no longer available."));
		return;
	}
	FString URL;
#if COMMONUSER_OSSV1
	const IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(SessionName, URL) || URL.IsEmpty())
	{
		FailOperation(TEXT("AddressUnavailable"), LOCTEXT("AddressUnavailable", "Could not resolve the host's connection address."));
		return;
	}
#else
	FailOperation(TEXT("UnsupportedBackend"), LOCTEXT("OSSv1Required", "This session flow requires OSSv1."));
	return;
#endif
	OnPreClientTravelEvent.Broadcast(URL);
	if (URL.IsEmpty())
	{
		FailOperation(TEXT("AddressUnavailable"), LOCTEXT("AddressUnavailable", "Could not resolve the host's connection address."));
		return;
	}
	bAwaitingTravel = true;
	TravelSourceWorld = GetWorld();
	Player->ClientTravel(URL, TRAVEL_Absolute);
}

void UCommonSessionSubsystem::NotifyUserRequestedSession(const FPlatformUserId& PlatformUserId, UCommonSession_SearchResult* RequestedSession, const FOnlineResultInformation& RequestedSessionResult)
{
	OnUserRequestedSessionEvent.Broadcast(PlatformUserId, RequestedSession, RequestedSessionResult);
	K2_OnUserRequestedSessionEvent.Broadcast(PlatformUserId, RequestedSession, RequestedSessionResult);
}

void UCommonSessionSubsystem::NotifyJoinSessionComplete(const FOnlineResultInformation& Result)
{
	OnJoinSessionCompleteEvent.Broadcast(Result);
	K2_OnJoinSessionCompleteEvent.Broadcast(Result);
}

void UCommonSessionSubsystem::NotifyCreateSessionComplete(const FOnlineResultInformation& Result)
{
	OnCreateSessionCompleteEvent.Broadcast(Result);
	K2_OnCreateSessionCompleteEvent.Broadcast(Result);
}

void UCommonSessionSubsystem::NotifySessionInformationUpdated(ECommonSessionInformationState SessionStatus, const FString& GameMode, const FString& MapName)
{
	OnSessionInformationChangedEvent.Broadcast(SessionStatus, GameMode, MapName);
	K2_OnSessionInformationChangedEvent.Broadcast(SessionStatus, GameMode, MapName);
}

void UCommonSessionSubsystem::NotifyDestroySessionRequested(const FPlatformUserId& PlatformUserId, const FName& SessionName)
{
	OnDestroySessionRequestedEvent.Broadcast(PlatformUserId, SessionName);
	K2_OnDestroySessionRequestedEvent.Broadcast(PlatformUserId, SessionName);
}

void UCommonSessionSubsystem::SetCreateSessionError(const FText& ErrorText)
{
	CreateSessionResult.bWasSuccessful = false;
	CreateSessionResult.ErrorId = TEXT("InternalFailure");

	// TODO May want to replace with a generic error text in shipping builds depending on how much data you want to give users
	CreateSessionResult.ErrorText = ErrorText;
}

void UCommonSessionSubsystem::CreateHostReservationBeacon()
{
	check(!BeaconHostListener.IsValid());
	check(!ReservationBeaconHost.IsValid());

	UWorld* const World = GetWorld();
	BeaconHostListener = World->SpawnActor<AOnlineBeaconHost>(AOnlineBeaconHost::StaticClass());
	check(BeaconHostListener.IsValid());
	verify(BeaconHostListener->InitHost());

	ReservationBeaconHost = World->SpawnActor<APartyBeaconHost>(APartyBeaconHost::StaticClass());
	check(ReservationBeaconHost.IsValid());

	if (ReservationBeaconHostState)
	{
		ReservationBeaconHost->InitFromBeaconState(&*ReservationBeaconHostState);
	}
	else
	{
		// TODO: We are using the default hard-coded values for the parameters for now, but they are configurable
		ReservationBeaconHost->InitHostBeacon(BeaconTeamCount, BeaconTeamSize, BeaconMaxReservations, NAME_GameSession);
		ReservationBeaconHostState = ReservationBeaconHost->GetState();
	}

	BeaconHostListener->RegisterHost(ReservationBeaconHost.Get());
	BeaconHostListener->PauseBeaconRequests(false);
}

void UCommonSessionSubsystem::DestroyHostReservationBeacon()
{
	if (BeaconHostListener.IsValid() && ReservationBeaconHost.IsValid())
	{
		BeaconHostListener->UnregisterHost(ReservationBeaconHost->GetBeaconType());
	}
	if (BeaconHostListener.IsValid())
	{
		BeaconHostListener->Destroy();
		BeaconHostListener = nullptr;
	}
	if (ReservationBeaconHost.IsValid())
	{
		ReservationBeaconHost->Destroy();
		ReservationBeaconHost = nullptr;
	}
}

#if COMMONUSER_OSSV1
void UCommonSessionSubsystem::HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	RecoverConnection(TEXT("SessionFailure"), LOCTEXT("LostOnlineSession", "The online session was disconnected."));
}

void UCommonSessionSubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 LocalUserIndex, FUniqueNetIdPtr AcceptingUserId, const FOnlineSessionSearchResult& SearchResult)
{
	FPlatformUserId PlatformUserId = IPlatformInputDeviceMapper::Get().GetPlatformUserForUserIndex(LocalUserIndex);

	UCommonSession_SearchResult* RequestedSession = nullptr;
	FOnlineResultInformation ResultInfo;
	if (bWasSuccessful)
	{
		RequestedSession = NewObject<UCommonSession_SearchResult>(this);
		RequestedSession->Result = SearchResult;
	}
	else
	{
		// No FOnlineError to initialize from
		ResultInfo.bWasSuccessful = false;
		ResultInfo.ErrorId = TEXT("failed"); // This is not robust but there is no extended information available
		ResultInfo.ErrorText = LOCTEXT("Error_SessionUserInviteAcceptedFailed", "Failed to handle the join request");
	}
	NotifyUserRequestedSession(PlatformUserId, RequestedSession, ResultInfo);
}

#endif // COMMONUSER_OSSV1

void UCommonSessionSubsystem::TravelLocalSessionFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ReasonString)
{
	if (World != GetWorld()) { return; }
	RecoverConnection(TEXT("TravelFailure"), FText::FromString(ReasonString));
}

void UCommonSessionSubsystem::HandlePostLoadMap(UWorld* World)
{
	// Ignore null worlds.
	if (!World)
	{
		return;
	}

	// Ignore any world that isn't part of this game instance, which can be the case in the editor.
	if (World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// We don't care about updating the session unless the world type is game/pie.
	if (!(World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE))
	{
		return;
	}

#if COMMONUSER_OSSV1
	const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	if (!SessionInterface.IsValid()) { return; }

	const FName SessionName(NAME_GameSession);
	FNamedOnlineSession* CurrentSession = SessionInterface->GetNamedSession(SessionName);
	if (bAwaitingTravel && World != TravelSourceWorld.Get())
	{
		const bool bConnected = ActiveOperation == EOperation::Joining ? World->GetNetMode() == NM_Client :
			(bOfflineTravel || World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer);
		if (bConnected)
		{
			FOnlineResultInformation Result;
			Result.bWasSuccessful = true;
			bOwnsPendingSession = false;
			FString Mode;
			if (CurrentSession) { CurrentSession->SessionSettings.Get(SETTING_GAMEMODE, Mode); }
			NotifySessionInformationUpdated(ECommonSessionInformationState::InGame, Mode, UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
			CompleteOperation(Result);
		}
		else
		{
			FailOperation(TEXT("ConnectionNotEstablished"), LOCTEXT("NoNetDriver", "The map loaded without the required network connection."));
			return;
		}
	}


	// If we're hosting a session, update the advertised map name.
	if (CurrentSession != nullptr && CurrentSession->bHosting)
	{
		// This needs to be the full package path to match the host GetMapName function, World->GetMapName is currently the short name - update host settings
		CurrentSession->SessionSettings.Set(SETTING_MAPNAME, UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()), EOnlineDataAdvertisementType::ViaOnlineService);

		SessionInterface->UpdateSession(SessionName, CurrentSession->SessionSettings, true);

		if (bUseBeacons)
		{
			CreateHostReservationBeacon();
		}
	}
#endif // COMMONUSER_OSSV1
}


bool UCommonSessionSubsystem::IsSessionBusy() const
{
	return ActiveOperation != EOperation::None || bWantToDestroyPendingSession || SearchSettings.IsValid();
}

bool UCommonSessionSubsystem::HasActiveSession() const
{
#if COMMONUSER_OSSV1
	if (!GetWorld()) { return false; }
	const IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
	return Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession) != nullptr;
#else
	return false;
#endif
}

void UCommonSessionSubsystem::BeginOperation(EOperation Operation)
{
	ActiveOperation = Operation;
	bAwaitingTravel = false;
	bOwnsPendingSession = false;
	bOfflineTravel = false;
	bLeftPreviousSession = false;
	OperationDeadline = FPlatformTime::Seconds() + 60.0;
}

void UCommonSessionSubsystem::CompleteOperation(const FOnlineResultInformation& Result)
{
	const EOperation Completed = ActiveOperation;
	ActiveOperation = EOperation::None;
	QuickPlaySearchRequest = nullptr;
	bLeftPreviousSession = false;
	bAwaitingTravel = false;
	OperationDeadline = 0.0;
	if (Completed == EOperation::Joining) { NotifyJoinSessionComplete(Result); }
	else if (Completed != EOperation::None) { NotifyCreateSessionComplete(Result); }
}

void UCommonSessionSubsystem::CancelActiveSearch(const FText& ErrorText)
{
#if COMMONUSER_OSSV1
	TSharedPtr<FCommonOnlineSearchSettings> CancelledSearch = MoveTemp(SearchSettings);
	if (!CancelledSearch) { return; }
	CancelledSearch->SearchRequest->OnSearchFinished.RemoveAll(this);
	if (UWorld* World = GetWorld())
	{
		if (const IOnlineSessionPtr Sessions = Online::GetSessionInterface(World)) { Sessions->CancelFindSessions(); }
	}
	CancelledSearch->SearchRequest->Results.Reset();
	CancelledSearch->SearchRequest->NotifySearchFinished(false, ErrorText);
#endif
}

void UCommonSessionSubsystem::FailOperation(const FString& ErrorId, const FText& ErrorText)
{
	FOnlineResultInformation Result;
	Result.bWasSuccessful = false;
	Result.ErrorId = ErrorId;
	Result.ErrorText = ErrorText;
	UE_LOG(LogCommonSession, Warning, TEXT("Session request failed (%s): %s"), *ErrorId, *ErrorText.ToString());
	// Set cleanup state before invoking observers so retries cannot overlap destruction.
	const bool bRecover = bAwaitingTravel || bLeftPreviousSession;
	const bool bCleanup = bOwnsPendingSession;
	// Destruction can outlive the join deadline. Remember why the queued join was
	// abandoned so its late successful destruction still returns to the frontend.
	if (QueuedJoin && bWantToDestroyPendingSession && !bLeftPreviousSession)
	{
		bRecoverAfterCleanup = true;
		DeferredCleanupFailure = Result;
	}
	if (bCleanup) { BeginCleanup(); }
	QueuedJoin = nullptr;
	CancelActiveSearch(ErrorText);
	CompleteOperation(Result);
	NotifySessionInformationUpdated(ECommonSessionInformationState::OutOfGame);
#if COMMONUSER_OSSV1
	if (bCleanup) { CleanUpSessionsOSSv1(); }
#endif
	if (bRecover) { OnSessionConnectionFailureEvent.Broadcast(Result); }
}

bool UCommonSessionSubsystem::RejectQuarantinedOperation()
{
	if (!bCleanupQuarantined) { return false; }
	FailOperation(TEXT("CleanupUnavailable"), LOCTEXT("CleanupQuarantined", "Steam is still leaving the previous session. Please wait, or restart the game if Steam does not recover."));
	return true;
}

void UCommonSessionSubsystem::BeginCleanup()
{
	if (bWantToDestroyPendingSession || bCleanupQuarantined) { return; }
	bWantToDestroyPendingSession = true;
	CleanupDeadline = FPlatformTime::Seconds() + 30.0;
	++CleanupGeneration;
#if COMMONUSER_OSSV1
	CleanupSessionInterface = Online::GetSessionInterface(GetWorld());
	if (CleanupSessionInterface)
	{
		CleanupEndHandle = CleanupSessionInterface->AddOnEndSessionCompleteDelegate_Handle(
			FOnEndSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnEndSessionComplete, CleanupGeneration));
		CleanupDestroyHandle = CleanupSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete, CleanupGeneration));
	}
#endif
}

void UCommonSessionSubsystem::ResetCleanupCallbacks()
{
#if COMMONUSER_OSSV1
	if (CleanupSessionInterface)
	{
		CleanupSessionInterface->ClearOnEndSessionCompleteDelegate_Handle(CleanupEndHandle);
		CleanupSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(CleanupDestroyHandle);
	}
	CleanupSessionInterface.Reset();
	CleanupEndHandle.Reset();
	CleanupDestroyHandle.Reset();
#endif
}

void UCommonSessionSubsystem::ExpireCleanup()
{
	FOnlineResultInformation Failure;
	Failure.bWasSuccessful = false;
	Failure.ErrorId = TEXT("CleanupTimeout");
	Failure.ErrorText = LOCTEXT("CleanupTimeout", "Leaving the previous session timed out. Returning to the main menu.");
#if COMMONUSER_OSSV1
	AbandonedSessionInterface = CleanupSessionInterface;
#endif
	ResetCleanupCallbacks();
	++CleanupGeneration;
	bCleanupQuarantined = true;
	bWantToDestroyPendingSession = false;
	bOwnsPendingSession = false;
	bRecoverAfterCleanup = false;
	QueuedJoin = nullptr;
	CleanupDeadline = 0.0;
	CancelActiveSearch(Failure.ErrorText);
	CompleteOperation(Failure);
	NotifySessionInformationUpdated(ECommonSessionInformationState::OutOfGame);
	OnSessionCleanupCompleteEvent.Broadcast(false);
	OnSessionConnectionFailureEvent.Broadcast(Failure);
}

void UCommonSessionSubsystem::TickAbandonedCleanup()
{
#if COMMONUSER_OSSV1
	if (!bCleanupQuarantined || !AbandonedSessionInterface) { return; }
	// Keep NAME_GameSession reserved until the old SDK tasks have actually removed it.
	// End/Destroy callbacks are detached; polling cannot complete a newer request.
	if (!AbandonedSessionInterface->GetNamedSession(NAME_GameSession))
	{
		AbandonedSessionInterface.Reset();
		bCleanupQuarantined = false;
		return;
	}
	const EOnlineSessionState::Type State = AbandonedSessionInterface->GetSessionState(NAME_GameSession);
	if (State == EOnlineSessionState::InProgress) { AbandonedSessionInterface->EndSession(NAME_GameSession); }
	else if (State == EOnlineSessionState::Ended || State == EOnlineSessionState::Pending)
	{
		AbandonedSessionInterface->DestroySession(NAME_GameSession);
	}
#endif
}

void UCommonSessionSubsystem::FinishCleanup(bool bSucceeded)
{
	if (bCleanupQuarantined) { return; }
	ResetCleanupCallbacks();
	CleanupDeadline = 0.0;
	const bool bDeferredRecovery = bRecoverAfterCleanup;
	const FOnlineResultInformation DeferredFailure = DeferredCleanupFailure;
	bRecoverAfterCleanup = false;
	bWantToDestroyPendingSession = false;
	bOwnsPendingSession = false;
	OnSessionCleanupCompleteEvent.Broadcast(bSucceeded);
	if (bDeferredRecovery && bSucceeded) { OnSessionConnectionFailureEvent.Broadcast(DeferredFailure); }
	if (QueuedJoin)
	{
		if (bSucceeded) { bLeftPreviousSession = true; ContinueQueuedJoin(); }
		else { FailOperation(TEXT("CleanupFailed"), LOCTEXT("CleanupFailed", "Could not leave the previous session. Please try again.")); }
	}
}

void UCommonSessionSubsystem::ContinueQueuedJoin()
{
	TStrongObjectPtr<UCommonSession_SearchResult> Request(QueuedJoin);
	QueuedJoin = nullptr;
	APlayerController* Player = JoiningPlayer.Get();
	if (!Player || !Player->GetLocalPlayer())
	{
		FailOperation(TEXT("InvalidPlayer"), LOCTEXT("TravelNoPlayer", "The local player is no longer available."));
		return;
	}
	JoinSessionInternal(Player->GetLocalPlayer(), Request.Get());
}

bool UCommonSessionSubsystem::TickOperation(float DeltaTime)
{
	TickAbandonedCleanup();
	if (bWantToDestroyPendingSession && CleanupDeadline > 0.0 && FPlatformTime::Seconds() >= CleanupDeadline)
	{
		ExpireCleanup();
		return true;
	}
#if COMMONUSER_OSSV1
	if (SearchSettings && (SearchSettings->SearchState == EOnlineAsyncTaskState::Done ||
		SearchSettings->SearchState == EOnlineAsyncTaskState::Failed))
	{
		OnFindSessionsComplete(SearchSettings->SearchState == EOnlineAsyncTaskState::Done);
	}
#endif
	if (ActiveOperation != EOperation::None && FPlatformTime::Seconds() >= OperationDeadline)
	{
		RecoverConnection(TEXT("Timeout"), LOCTEXT("SessionTimeout", "The session request timed out. Please try again."));
	}
	return true;
}

void UCommonSessionSubsystem::NetworkSessionFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& Reason)
{
	// PendingConnectionFailure is broadcast with a null World before the first map loads.
	const FWorldContext* PendingContext = GEngine && NetDriver ? GEngine->GetWorldContextFromPendingNetGameNetDriver(NetDriver) : nullptr;
	if (!bWantToDestroyPendingSession && ((World && World->GetGameInstance() == GetGameInstance()) ||
		(PendingContext && PendingContext->OwningGameInstance == GetGameInstance())))
	{
		RecoverConnection(TEXT("NetworkFailure"), FText::FromString(Reason));
	}
}

void UCommonSessionSubsystem::RecoverConnection(const FString& ErrorId, const FText& ErrorText)
{
	if (bCleanupQuarantined) { return; }
	if (ActiveOperation != EOperation::None)
	{
		FailOperation(ErrorId, ErrorText);
		return;
	}
	if (!HasActiveSession() || bWantToDestroyPendingSession) { return; }
	FOnlineResultInformation Result;
	Result.bWasSuccessful = false;
	Result.ErrorId = ErrorId;
	Result.ErrorText = ErrorText;
	CleanUpSessions();
	OnSessionConnectionFailureEvent.Broadcast(Result);
}

#if WITH_DEV_AUTOMATION_TESTS && COMMONUSER_OSSV1
#include "Misc/AutomationTest.h"
#include "Engine/PendingNetGame.h"
#include "IpNetDriver.h"
#include "Misc/ScopeExit.h"
#include "AssetRegistry/AssetBundleData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSessionRequestLifecycleTest, "CommonUser.Session.RequestLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonSessionRequestLifecycleTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<UCommonSessionSubsystem> Session(NewObject<UCommonSessionSubsystem>(Instance.Get()));
	int32 HostCompletions = 0;
	int32 JoinCompletions = 0;
	Session->OnCreateSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++HostCompletions;
		TestFalse(TEXT("Invalid host requests fail"), Result.bWasSuccessful);
		TestFalse(TEXT("Operation cleared before notifying observers"), Session->IsSessionBusy());
	});
	Session->OnJoinSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++JoinCompletions;
		TestFalse(TEXT("Invalid joins/expired requests never report success"), Result.bWasSuccessful);
	});
	Session->HostSession(nullptr, nullptr);
	TestEqual(TEXT("Invalid host completes exactly once"), HostCompletions, 1);
	Session->JoinSession(nullptr, nullptr);
	TestEqual(TEXT("Invalid join completes exactly once"), JoinCompletions, 1);
	Session->QuickPlaySession(nullptr, nullptr);
	TestEqual(TEXT("Invalid quickplay notifies the host observer"), HostCompletions, 2);

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->OperationDeadline = 0;
	Session->TickOperation(0);
	Session->TickOperation(0);
	TestEqual(TEXT("Timeout completes exactly once despite subsequent ticks"), JoinCompletions, 2);
	TestFalse(TEXT("Timed-out request releases operation"), Session->IsSessionBusy());

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->InternalTravelToSession(NAME_GameSession);
	TestEqual(TEXT("Missing travel controller completes join with failure"), JoinCompletions, 3);

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->QueuedJoin = NewObject<UCommonSession_SearchResult>(Session.Get());
	Session->bWantToDestroyPendingSession = true;
	Session->FinishCleanup(false);
	TestEqual(TEXT("Failed destruction fails the queued join exactly once"), JoinCompletions, 4);
	TestNull(TEXT("Failed cleanup discards queued invitation"), Session->QueuedJoin.Get());

	TStrongObjectPtr<UWorld> PendingWorld(NewObject<UWorld>());
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(PendingWorld.Get());
	Context.OwningGameInstance = Instance.Get();
	Context.PendingNetGame = NewObject<UPendingNetGame>();
	Context.PendingNetGame->NetDriver = NewObject<UIpNetDriver>();
	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->bAwaitingTravel = true;
	int32 ConnectionFailures = 0;
	Session->OnSessionConnectionFailureEvent.AddLambda([&](const FOnlineResultInformation&) { ++ConnectionFailures; });
	Session->NetworkSessionFailure(nullptr, Context.PendingNetGame->NetDriver, ENetworkFailure::PendingConnectionFailure, TEXT("Test connection refused"));
	TestEqual(TEXT("Null-world pending connection failure completes request"), JoinCompletions, 5);
	TestEqual(TEXT("Pending failure requests frontend recovery"), ConnectionFailures, 1);
	GEngine->DestroyWorldContext(PendingWorld.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSessionSearchReentryTest, "CommonUser.Session.SearchReentry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonSessionSearchReentryTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<UCommonSessionSubsystem> Session(NewObject<UCommonSessionSubsystem>(Instance.Get()));
	UCommonSession_SearchSessionRequest* First = NewObject<UCommonSession_SearchSessionRequest>(Session.Get());
	UCommonSession_SearchSessionRequest* Second = NewObject<UCommonSession_SearchSessionRequest>(Session.Get());
	const TSharedRef<FCommonOnlineSearchSettingsOSSv1> FirstSettings = MakeShared<FCommonOnlineSearchSettingsOSSv1>(First);
	const TSharedRef<FCommonOnlineSearchSettingsOSSv1> SecondSettings = MakeShared<FCommonOnlineSearchSettingsOSSv1>(Second);
	int32 FirstNotified = 0;
	int32 SecondNotified = 0;
	First->OnSearchFinished.AddLambda([&](bool, const FText&) { ++FirstNotified; });
	Second->OnSearchFinished.AddLambda([&](bool bSucceeded, const FText&)
	{
		++SecondNotified;
		TestFalse(TEXT("Overlapping search is rejected"), bSucceeded);
	});
	Session->SearchSettings = FirstSettings;
	Session->FindSessionsInternal(nullptr, SecondSettings);
	TestEqual(TEXT("Original search is not falsely notified"), FirstNotified, 0);
	TestEqual(TEXT("Only incoming request receives rejection"), SecondNotified, 1);
	TestTrue(TEXT("Original settings remain owned"), Session->SearchSettings == FirstSettings);

	First->OnSearchFinished.AddLambda([&](bool, const FText&)
	{
		TestFalse(TEXT("Completed search is released before callback"), Session->SearchSettings.IsValid());
		// Represents a fresh search synchronously started by a UI completion handler.
		Session->SearchSettings = SecondSettings;
	});
	FirstSettings->SearchState = EOnlineAsyncTaskState::Done;
	Session->OnFindSessionsComplete(true);
	TestEqual(TEXT("Original search completes once"), FirstNotified, 1);
	TestTrue(TEXT("Reentrant replacement survives completion"), Session->SearchSettings == SecondSettings);
	Session->SearchSettings.Reset();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSessionRecoveryRegressionTest, "CommonUser.Session.RecoveryRegressions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonSessionRecoveryRegressionTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<UCommonSessionSubsystem> Session(NewObject<UCommonSessionSubsystem>(Instance.Get()));
	int32 RecoveryCount = 0;
	int32 JoinCount = 0;
	Session->OnSessionConnectionFailureEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++RecoveryCount;
		TestFalse(TEXT("Recovery reports failure"), Result.bWasSuccessful);
	});
	Session->OnJoinSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation&) { ++JoinCount; });

	// Exercise the actual cleanup continuation: the controller disappears while leaving A.
	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->QueuedJoin = NewObject<UCommonSession_SearchResult>(Session.Get());
	Session->bWantToDestroyPendingSession = true;
	Session->FinishCleanup(true);
	TestEqual(TEXT("Failure after leaving old session requests frontend recovery"), RecoveryCount, 1);
	TestEqual(TEXT("Cleanup continuation completes join exactly once"), JoinCount, 1);

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->bLeftPreviousSession = true;
	Session->OnJoinSessionComplete(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
	Session->OnJoinSessionComplete(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
	TestEqual(TEXT("Rejected replacement lobby recovers once before travel"), RecoveryCount, 2);
	TestEqual(TEXT("Duplicate join callback does not complete twice"), JoinCount, 2);

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->bLeftPreviousSession = true;
	Session->InternalTravelToSession(NAME_GameSession);
	TestEqual(TEXT("Replacement travel setup failure requests recovery"), RecoveryCount, 3);

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->bLeftPreviousSession = true;
	Session->OperationDeadline = 0;
	Session->TickOperation(0);
	Session->TickOperation(0);
	TestEqual(TEXT("Replacement join timeout requests recovery once"), RecoveryCount, 4);

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->QueuedJoin = NewObject<UCommonSession_SearchResult>(Session.Get());
	Session->bWantToDestroyPendingSession = true;
	Session->FinishCleanup(false);
	TestEqual(TEXT("Failure to leave old session does not force recovery"), RecoveryCount, 4);

	Session->BeginOperation(UCommonSessionSubsystem::EOperation::QuickPlay);
	UCommonSession_SearchSessionRequest* OldRequest = NewObject<UCommonSession_SearchSessionRequest>(Session.Get());
	const auto OldSettings = MakeShared<FCommonOnlineSearchSettingsOSSv1>(OldRequest);
	OldSettings->SearchState = EOnlineAsyncTaskState::InProgress;
	Session->SearchSettings = OldSettings;
	Session->QuickPlaySearchRequest = OldRequest;
	OldRequest->OnSearchFinished.AddUObject(Session.Get(), &UCommonSessionSubsystem::HandleQuickPlaySearchFinished,
		TWeakObjectPtr<APlayerController>(), TStrongObjectPtr<UCommonSession_HostSessionRequest>(), OldRequest);
	int32 SearchFailures = 0;
	OldRequest->OnSearchFinished.AddLambda([&](bool bSuccess, const FText&)
	{
		++SearchFailures;
		TestFalse(TEXT("Timed-out search reports failure"), bSuccess);
		TestFalse(TEXT("Search ownership released before cancellation notification"), Session->SearchSettings.IsValid());
	});
	int32 QuickPlayCompletions = 0;
	Session->OnCreateSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++QuickPlayCompletions;
		TestEqual(TEXT("Quick Play retains timeout error"), Result.ErrorId, FString(TEXT("Timeout")));
		TestFalse(TEXT("Timed-out Quick Play releases Busy before UI completion"), Session->IsSessionBusy());
	});
	Session->OperationDeadline = 0;
	Session->TickOperation(0);
	Session->TickOperation(0);
	TestEqual(TEXT("Timed-out search completes once"), SearchFailures, 1);
	TestEqual(TEXT("Timed-out Quick Play completes once"), QuickPlayCompletions, 1);

	// A retry starts before the cancelled Steam query emits its global completion.
	Session->BeginOperation(UCommonSessionSubsystem::EOperation::QuickPlay);
	UCommonSession_SearchSessionRequest* RetryRequest = NewObject<UCommonSession_SearchSessionRequest>(Session.Get());
	const auto RetrySettings = MakeShared<FCommonOnlineSearchSettingsOSSv1>(RetryRequest);
	RetrySettings->SearchState = EOnlineAsyncTaskState::InProgress;
	Session->SearchSettings = RetrySettings;
	Session->QuickPlaySearchRequest = RetryRequest;
	int32 RetryCompletions = 0;
	RetryRequest->OnSearchFinished.AddLambda([&](bool bSuccess, const FText&)
	{
		++RetryCompletions;
		TestTrue(TEXT("Retry uses its own successful state despite stale failure callback"), bSuccess);
	});
	OldSettings->SearchState = EOnlineAsyncTaskState::Failed;
	Session->OnFindSessionsComplete(false);
	Session->HandleQuickPlaySearchFinished(false, FText(), TWeakObjectPtr<APlayerController>(),
		TStrongObjectPtr<UCommonSession_HostSessionRequest>(), OldRequest);
	TestTrue(TEXT("Late callbacks preserve the retry search"), Session->SearchSettings == RetrySettings);
	TestEqual(TEXT("Late callbacks do not finish retry"), RetryCompletions, 0);
	TestEqual(TEXT("Late callbacks do not fail retry operation"), QuickPlayCompletions, 1);
	RetrySettings->SearchState = EOnlineAsyncTaskState::Done;
	Session->OnFindSessionsComplete(false);
	Session->OnFindSessionsComplete(true);
	TestEqual(TEXT("Retry completes exactly once using its own results"), RetryCompletions, 1);
	Session->ActiveOperation = UCommonSessionSubsystem::EOperation::None;
	Session->QuickPlaySearchRequest = nullptr;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSessionCleanupTimeoutTest, "CommonUser.Session.CleanupTimeoutRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonSessionCleanupTimeoutTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<UCommonSessionSubsystem> Session(NewObject<UCommonSessionSubsystem>(Instance.Get()));
	int32 Joins = 0;
	int32 Recoveries = 0;
	Session->OnJoinSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++Joins;
		TestEqual(TEXT("Join preserves timeout result"), Result.ErrorId, FString(TEXT("Timeout")));
	});
	Session->OnSessionConnectionFailureEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++Recoveries;
		TestEqual(TEXT("Late cleanup recovery preserves timeout reason"), Result.ErrorId, FString(TEXT("Timeout")));
	});
	auto TimeoutDuringCleanup = [&]()
	{
		Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
		Session->QueuedJoin = NewObject<UCommonSession_SearchResult>(Session.Get());
		Session->bWantToDestroyPendingSession = true;
		Session->OperationDeadline = 0;
		Session->TickOperation(0);
		Session->TickOperation(0);
		TestNull(TEXT("Timeout discards invitation without starting it"), Session->QueuedJoin.Get());
		TestTrue(TEXT("Cleanup remains busy until backend finishes"), Session->IsSessionBusy());
	};
	TimeoutDuringCleanup();
	TestEqual(TEXT("Timeout completes join once"), Joins, 1);
	TestEqual(TEXT("Recovery waits for cleanup result"), Recoveries, 0);
	Session->FinishCleanup(true);
	Session->FinishCleanup(true);
	TestEqual(TEXT("Late successful cleanup requests recovery exactly once"), Recoveries, 1);
	TestEqual(TEXT("Cleanup does not complete join again"), Joins, 1);
	TestFalse(TEXT("Completed cleanup releases Busy"), Session->IsSessionBusy());

	TimeoutDuringCleanup();
	Session->FinishCleanup(false);
	TestEqual(TEXT("Failed cleanup leaves existing world alone"), Recoveries, 1);
	TestFalse(TEXT("Failed cleanup clears deferred recovery"), Session->bRecoverAfterCleanup);
	Session->FinishCleanup(true);
	TestEqual(TEXT("Unrelated cleanup cannot replay stale recovery"), Recoveries, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSessionCleanupWatchdogTest, "CommonUser.Session.CleanupWatchdog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonSessionCleanupWatchdogTest::RunTest(const FString& Parameters)
{
	IOnlineSubsystem* NullOSS = IOnlineSubsystem::Get(FName(TEXT("NULL")));
	const IOnlineSessionPtr Backend = NullOSS ? NullOSS->GetSessionInterface() : nullptr;
	if (!TestTrue(TEXT("OSSv1 NULL backend is available for deterministic cleanup tests"), Backend.IsValid())) { return false; }
	if (!TestNull(TEXT("Test backend has no existing GameSession"), Backend->GetNamedSession(NAME_GameSession))) { return false; }
	ON_SCOPE_EXIT { Backend->RemoveNamedSession(NAME_GameSession); };

	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<UCommonSessionSubsystem> Session(NewObject<UCommonSessionSubsystem>(Instance.Get()));
	int32 Recoveries = 0;
	int32 CleanupCompletions = 0;
	int32 JoinCompletions = 0;
	int32 RetryRejections = 0;
	Session->OnSessionConnectionFailureEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++Recoveries;
		TestEqual(TEXT("Watchdog reports cleanup timeout"), Result.ErrorId, FString(TEXT("CleanupTimeout")));
		TestFalse(TEXT("Recovery observes released Busy state"), Session->IsSessionBusy());
	});
	Session->OnSessionCleanupCompleteEvent.AddLambda([&](bool) { ++CleanupCompletions; });
	Session->OnJoinSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		if (Result.ErrorId == TEXT("CleanupUnavailable")) { ++RetryRejections; }
		else { ++JoinCompletions; TestEqual(TEXT("Pending join fails with cleanup timeout"), Result.ErrorId, FString(TEXT("CleanupTimeout"))); }
	});
	Session->OnCreateSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		if (Result.ErrorId == TEXT("CleanupUnavailable")) { ++RetryRejections; }
	});

	FOnlineSessionSettings Settings;
	Settings.bShouldAdvertise = false;
	if (!TestTrue(TEXT("Create deterministic backend session"), Backend->CreateSession(0, NAME_GameSession, Settings))) { return false; }
	FNamedOnlineSession* OldSession = Backend->GetNamedSession(NAME_GameSession);
	if (!TestNotNull(TEXT("Backend owns test session"), OldSession)) { return false; }
	OldSession->SessionState = EOnlineSessionState::Destroying;
	Session->BeginOperation(UCommonSessionSubsystem::EOperation::Joining);
	Session->QueuedJoin = NewObject<UCommonSession_SearchResult>(Session.Get());
	Session->BeginCleanup();
	Session->ResetCleanupCallbacks();
	Session->CleanupSessionInterface = Backend;
	const uint64 OldGeneration = Session->CleanupGeneration;
	const FOnEndSessionCompleteDelegate OldEndCallback = FOnEndSessionCompleteDelegate::CreateUObject(
		Session.Get(), &UCommonSessionSubsystem::OnEndSessionComplete, OldGeneration);
	const FOnDestroySessionCompleteDelegate OldDestroyCallback = FOnDestroySessionCompleteDelegate::CreateUObject(
		Session.Get(), &UCommonSessionSubsystem::OnDestroySessionComplete, OldGeneration);
	Session->CleanupEndHandle = Backend->AddOnEndSessionCompleteDelegate_Handle(OldEndCallback);
	Session->CleanupDestroyHandle = Backend->AddOnDestroySessionCompleteDelegate_Handle(OldDestroyCallback);
	const double OriginalDeadline = Session->CleanupDeadline;
	Session->BeginCleanup();
	TestEqual(TEXT("Repeated cleanup does not extend its deadline"), Session->CleanupDeadline, OriginalDeadline);
	Session->CleanupDeadline = FPlatformTime::Seconds() - 1;
	Session->TickOperation(0);
	Session->TickOperation(0);
	TestEqual(TEXT("Missing callback completes join once"), JoinCompletions, 1);
	TestEqual(TEXT("Missing callback reports cleanup failure once"), CleanupCompletions, 1);
	TestEqual(TEXT("Missing callback requests frontend recovery once"), Recoveries, 1);
	TestNull(TEXT("Timed-out invitation is not replayed"), Session->QueuedJoin.Get());
	TestFalse(TEXT("No permanent Busy after timeout"), Session->IsSessionBusy());
	TestTrue(TEXT("Backend remains isolated while old named session exists"), Session->bCleanupQuarantined);

	Backend->TriggerOnEndSessionCompleteDelegates(NAME_GameSession, true);
	Backend->TriggerOnDestroySessionCompleteDelegates(NAME_GameSession, true);
	OldEndCallback.Execute(NAME_GameSession, true);
	OldDestroyCallback.Execute(NAME_GameSession, true);
	Session->FinishCleanup(true);
	Session->CleanUpSessions();
	Session->HostSession(nullptr, nullptr);
	Session->JoinSession(nullptr, nullptr);
	Session->QuickPlaySession(nullptr, nullptr);
	TestEqual(TEXT("Retries fail explicitly while backend is quarantined"), RetryRejections, 3);
	TestEqual(TEXT("Old callbacks cannot replay cleanup completion"), CleanupCompletions, 1);
	TestEqual(TEXT("Old callbacks do not repeat frontend recovery"), Recoveries, 1);
	TestTrue(TEXT("Retry cannot destroy or replace old backend session"), Backend->GetNamedSession(NAME_GameSession) == OldSession);

	OldSession->SessionState = EOnlineSessionState::NoSession;
	Session->TickOperation(0);
	TestTrue(TEXT("NoSession state alone cannot retire an in-flight named session"), Session->bCleanupQuarantined);

	// Old SDK work finally progresses. Polling destroys it without reusing its callbacks.
	OldSession->SessionState = EOnlineSessionState::Pending;
	Session->TickOperation(0);
	Session->TickOperation(0);
	TestFalse(TEXT("Quarantine releases only after backend removed the old session"), Session->bCleanupQuarantined);
	TestNull(TEXT("Abandoned named session was removed"), Backend->GetNamedSession(NAME_GameSession));
	TestEqual(TEXT("Backend retirement does not replay recovery"), Recoveries, 1);

	// Captured delegates from generation A must not complete a later cleanup B.
	Session->BeginCleanup();
	const uint64 NewGeneration = Session->CleanupGeneration;
	Session->OnEndSessionComplete(NAME_GameSession, true, OldGeneration);
	Session->OnDestroySessionComplete(NAME_GameSession, true, OldGeneration);
	TestTrue(TEXT("Old generation does not finish new cleanup"), Session->bWantToDestroyPendingSession);
	Session->OnDestroySessionComplete(NAME_GameSession, true, NewGeneration);
	TestEqual(TEXT("New cleanup completes through its own callback"), CleanupCompletions, 2);
	TestFalse(TEXT("New cleanup releases Busy"), Session->IsSessionBusy());

	// No active join is needed: an already-cancelled cleanup has its own deadline.
	Session->BeginCleanup();
	Session->ResetCleanupCallbacks();
	Session->CleanupSessionInterface = Backend;
	Session->CleanupDeadline = FPlatformTime::Seconds() - 1;
	Session->TickOperation(0);
	TestEqual(TEXT("Cleanup watchdog runs with ActiveOperation None"), Recoveries, 2);
	TestEqual(TEXT("Idle cleanup does not complete the old join again"), JoinCompletions, 1);
	Session->TickOperation(0);
	TestFalse(TEXT("Already-idle backend retires quarantine on next tick"), Session->bCleanupQuarantined);
	return true;
}

// Run in an isolated editor process with a runtime-only DefaultPlatformService=NULL
// override. This performs real OSSv1 creation and listen-server map travel.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSessionCleanupRecoveryHostTest, "CommonUser.Integration.CleanupRecoveryHost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonSessionCleanupRecoveryHostTest::RunTest(const FString& Parameters)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS || OSS->GetSubsystemName() != FName(TEXT("NULL")))
	{
		AddError(TEXT("Run this integration fixture in an isolated editor with -ini:Engine:[OnlineSubsystem]:DefaultPlatformService=NULL"));
		return false;
	}
	const IOnlineSessionPtr Backend = OSS->GetSessionInterface();
	if (!TestTrue(TEXT("OSSv1 integration backend exists"), Backend.IsValid()) ||
		!TestNull(TEXT("Fixture does not overwrite an existing session"), Backend->GetNamedSession(NAME_GameSession))) { return false; }

	UWorld* SavedWorld = GWorld;
	const TArray<FNetDriverDefinition> SavedDrivers = GEngine->NetDriverDefinitions;
	for (FNetDriverDefinition& Driver : GEngine->NetDriverDefinitions)
	{
		if (Driver.DefName == NAME_GameNetDriver)
		{
			Driver.DriverClassName = FName(TEXT("/Script/OnlineSubsystemUtils.IpNetDriver"));
			Driver.DriverClassNameFallback = Driver.DriverClassName;
		}
	}
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
	Instance->InitializeStandalone(FName(TEXT("CleanupRecoveryHostFixture")));
	FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(Instance->GetWorld());
	Context.PIEInstance = 6777; // Force an independent copy if Entry is already the editor world.
	const FPrimaryAssetId MapId(FPrimaryAssetType(TEXT("AutomationCleanupRecoveryHost")), FName(TEXT("CleanupRecoveryEntry")));
	ON_SCOPE_EXIT
	{
		Backend->RemoveNamedSession(NAME_GameSession);
		UAssetManager::Get().RemovePrimaryAssetType(MapId.PrimaryAssetType);
		UWorld* World = Instance->GetWorld();
		Instance->Shutdown();
		if (World)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}
		GEngine->NetDriverDefinitions = SavedDrivers;
		GWorld = SavedWorld;
	};
	UCommonSessionSubsystem* Session = Instance->GetSubsystem<UCommonSessionSubsystem>();
	if (!TestNotNull(TEXT("Fixture uses the initialized production session subsystem"), Session)) { return false; }
	Session->bIsDedicatedServer = true; // Exercise the supported server-host path without a viewport/player.
	Session->bUseBeacons = false;
	TestTrue(TEXT("Fixture resolves the same OSS backend as public HostSession"), Online::GetSessionInterface(Instance->GetWorld()) == Backend);
	FAssetBundleData MapBundle;
	MapBundle.AddBundleAsset(FName(TEXT("AutomationMap")), FTopLevelAssetPath(TEXT("/Engine/Maps/Entry.Entry")));
	UAssetManager::Get().AddDynamicAsset(MapId, FSoftObjectPath(TEXT("/Engine/Maps/Entry.Entry")), MapBundle);
	TStrongObjectPtr<UCommonSession_HostSessionRequest> Request(Session->CreateOnlineHostSessionRequest());
	Request->MapID = MapId;
	Request->OnlineMode = ECommonSessionOnlineMode::LAN;
	Request->bUseLobbies = false;
	Request->bUsePresence = false;
	Request->MaxPlayerCount = 2;
	Request->ExtraArgs.Add(TEXT("game"), TEXT("/Script/Engine.GameModeBase"));
	FText ValidationError;
	if (!TestTrue(TEXT("Host request resolves a real engine map"), Request->ValidateAndLogErrors(ValidationError))) { return false; }

	int32 Recoveries = 0;
	int32 RejectedRetries = 0;
	int32 HostSuccesses = 0;
	const FDelegateHandle RecoveryHandle = Session->OnSessionConnectionFailureEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		++Recoveries;
		TestEqual(TEXT("Cleanup timeout triggers recovery"), Result.ErrorId, FString(TEXT("CleanupTimeout")));
	});
	const FDelegateHandle HostHandle = Session->OnCreateSessionCompleteEvent.AddLambda([&](const FOnlineResultInformation& Result)
	{
		if (Result.bWasSuccessful) { ++HostSuccesses; }
		else if (Result.ErrorId == TEXT("CleanupUnavailable")) { ++RejectedRetries; }
		else { AddError(FString::Printf(TEXT("Unexpected host failure: %s: %s"), *Result.ErrorId, *Result.ErrorText.ToString())); }
	});
	ON_SCOPE_EXIT
	{
		Session->OnSessionConnectionFailureEvent.Remove(RecoveryHandle);
		Session->OnCreateSessionCompleteEvent.Remove(HostHandle);
	};

	FOnlineSessionSettings OldSettings;
	OldSettings.bShouldAdvertise = false;
	if (!TestTrue(TEXT("Create the old backend session"), Backend->CreateSession(0, NAME_GameSession, OldSettings))) { return false; }
	FNamedOnlineSession* OldSession = Backend->GetNamedSession(NAME_GameSession);
	if (!TestNotNull(TEXT("Old backend session exists"), OldSession)) { return false; }
	OldSession->SessionState = EOnlineSessionState::Destroying; // Fault: backend callback never arrives.
	Session->CleanUpSessions();
	const uint64 OldGeneration = Session->CleanupGeneration;
	Session->CleanupDeadline = FPlatformTime::Seconds() - 1;
	Session->TickOperation(0);
	TestEqual(TEXT("Watchdog requests frontend recovery exactly once"), Recoveries, 1);
	TestFalse(TEXT("Watchdog releases UI Busy"), Session->IsSessionBusy());
	Session->HostSession(nullptr, Request.Get());
	TestEqual(TEXT("Public host is explicitly rejected before backend recovery"), RejectedRetries, 1);

	OldSession->SessionState = EOnlineSessionState::Pending; // Backend recovers and allows destruction.
	Session->TickOperation(0);
	Session->TickOperation(0);
	if (!TestFalse(TEXT("Backend retirement releases quarantine"), Session->bCleanupQuarantined)) { return false; }
	TestNull(TEXT("Old named session was actually removed"), Backend->GetNamedSession(NAME_GameSession));
	const TWeakObjectPtr<UWorld> SourceWorld = Instance->GetWorld();
	Session->HostSession(nullptr, Request.Get()); // Real validation, OSS create delegate, and ServerTravel.
	if (!TestNotNull(TEXT("Retry created a new backend session"), Backend->GetNamedSession(NAME_GameSession))) { return false; }
	TestEqual(TEXT("SDK creation alone does not report success"), HostSuccesses, 0);
	TestTrue(TEXT("Retry waits for destination travel"), Session->bAwaitingTravel);
	Session->OnEndSessionComplete(NAME_GameSession, true, OldGeneration);
	Session->OnDestroySessionComplete(NAME_GameSession, true, OldGeneration);
	TestNotNull(TEXT("Old callbacks preserve the new backend session"), Backend->GetNamedSession(NAME_GameSession));
	for (int32 Attempt = 0; Attempt < 3 && HostSuccesses == 0; ++Attempt) { GEngine->TickWorldTravel(Context, 1.0f); }
	TestEqual(TEXT("Retry reports host success once after real map travel"), HostSuccesses, 1);
	UWorld* Destination = Instance->GetWorld();
	if (!TestNotNull(TEXT("Destination world exists"), Destination)) { return false; }
	TestTrue(TEXT("Travel replaced the source world"), Destination != SourceWorld.Get());
	TestEqual(TEXT("Loaded the requested map"), UWorld::RemovePIEPrefix(Destination->GetOutermost()->GetName()), FString(TEXT("/Engine/Maps/Entry")));
	TestTrue(TEXT("Destination has a live listen-server net driver"), Destination->GetNetDriver() && Destination->GetNetMode() == NM_ListenServer);
	TestTrue(TEXT("New online session remains active after success"), Session->HasActiveSession());
	TestFalse(TEXT("Successful retry releases Busy"), Session->IsSessionBusy());
	Session->OnDestroySessionComplete(NAME_GameSession, true, OldGeneration);
	Session->TickOperation(0);
	TestEqual(TEXT("Late callbacks do not repeat success"), HostSuccesses, 1);
	TestEqual(TEXT("Retry does not repeat old recovery"), Recoveries, 1);
	TestNotNull(TEXT("Successful retry session survives old callbacks"), Backend->GetNamedSession(NAME_GameSession));
	return true;
}

#endif

#undef LOCTEXT_NAMESPACE
