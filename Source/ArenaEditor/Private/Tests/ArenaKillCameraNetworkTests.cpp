// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ArenaKillCameraTestPawn.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Camera/ArenaPlayerCameraManager.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "PlayInEditorDataTypes.h"
#include "Player/ArenaPlayerController.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"

namespace ArenaKillCameraNetworkTests
{
	struct FFixture
	{
		TArray<FNetDriverDefinition> SavedDrivers = GEngine->NetDriverDefinitions;
		FSoftClassPath SavedGameInstance = GetDefault<UGameMapsSettings>()->GameInstanceClass;

		~FFixture()
		{
			GEngine->NetDriverDefinitions = SavedDrivers;
			GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = SavedGameInstance;
		}
	};

	void ApplyLethalDamage(AArenaKillCameraTestPawn* Source, AArenaKillCameraTestPawn* Victim)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>();
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UArenaHealthSet::GetDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(200.0f);
		FGameplayEffectSpec Spec(Effect, Source->AbilitySystem->MakeEffectContext(), 1.0f);
		Source->AbilitySystem->ApplyGameplayEffectSpecToTarget(Spec, Victim->AbilitySystem);
	}

	class FCheckCamera : public IAutomationLatentCommand
	{
	public:
		explicit FCheckCamera(FAutomationTestBase* InTest) : Test(InTest) {}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > 45.0)
			{
				Test->AddError(FString::Printf(TEXT("Kill camera network test timed out at stage %d"), Stage));
				return true;
			}
			UWorld* Server = nullptr;
			UWorld* Client = nullptr;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType == EWorldType::PIE && World && World->HasBegunPlay())
				{
					if (World->GetNetMode() == NM_ListenServer)
					{
						Server = World;
					}
					else if (World->GetNetMode() == NM_Client)
					{
						Client = World;
					}
				}
			}
			if (!Server || !Client)
			{
				return false;
			}
			AArenaPlayerController* ClientController = Cast<AArenaPlayerController>(Client->GetFirstPlayerController());
			AArenaPlayerController* HostController = Cast<AArenaPlayerController>(Server->GetFirstPlayerController());
			if (!ClientController || !HostController)
			{
				return false;
			}
			AArenaPlayerCameraManager* ClientCamera = Cast<AArenaPlayerCameraManager>(ClientController->PlayerCameraManager);
			if (!ClientCamera)
			{
				return false;
			}
			if (Stage == 0)
			{
				for (FConstPlayerControllerIterator It = Server->GetPlayerControllerIterator(); It; ++It)
				{
					if (!It->Get()->IsLocalController())
					{
						VictimController = Cast<AArenaPlayerController>(It->Get());
					}
				}
				AArenaKillCameraTestPawn* Victim = VictimController.IsValid() ? Cast<AArenaKillCameraTestPawn>(VictimController->GetPawn()) : nullptr;
				AArenaKillCameraTestPawn* Killer = Cast<AArenaKillCameraTestPawn>(HostController->GetPawn());
				if (!Victim || !Killer || !ClientController->GetPawn())
				{
					return false;
				}
				OriginalKiller = Killer;
				Killer->SetActorLocation(FVector(1500.0, 0.0, 200.0));
				ApplyLethalDamage(Killer, Victim);
				Stage = 1;
				return false;
			}
			if (Stage == 1)
			{
				if (!ClientCamera->IsKillCameraActive())
				{
					return false;
				}
				const AArenaPlayerCameraManager* HostCamera = Cast<AArenaPlayerCameraManager>(HostController->PlayerCameraManager);
				Test->TestTrue(TEXT("Only the victim receives the reliable death view RPC"), HostCamera && !HostCamera->IsKillCameraActive());
				Test->TestEqual(TEXT("Server retains the original killer view target"), VictimController->GetViewTarget(), static_cast<AActor*>(OriginalKiller.Get()));
				OriginalKiller->SetNetCullDistanceSquared(10000.0f);
				OriginalKiller->SetActorLocation(FVector(100000.0, 60000.0, 5000.0));
				OriginalKiller->ForceNetUpdate();
				Stage = 2;
				return false;
			}
			if (Stage == 2)
			{
				AArenaKillCameraTestPawn* ClientKiller = nullptr;
				for (TActorIterator<AArenaKillCameraTestPawn> It(Client); It; ++It)
				{
					if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == HostController->PlayerState->GetPlayerId())
					{
						ClientKiller = *It;
						break;
					}
				}
				if (!ClientKiller || ClientKiller->GetActorLocation().X < 90000.0)
				{
					return false;
				}
				const FVector ExpectedDirection = (ClientKiller->GetActorLocation() - ClientCamera->GetCameraLocation()).GetSafeNormal();
				if (FVector::DotProduct(ClientCamera->GetCameraRotation().Vector(), ExpectedDirection) < 0.995)
				{
					return false;
				}
				Test->TestTrue(TEXT("Distant killer movement continues replicating to the victim"), ClientCamera->IsKillCameraActive());
				Test->TestTrue(TEXT("Remote camera stays at the victim while looking at the distant killer"),
					FMath::Abs(ClientCamera->GetCameraLocation().X - ClientController->GetPawn()->GetActorLocation().X) < 500.0);
				Test->TestEqual(TEXT("Remote camera retains the victim FOV"), ClientCamera->GetFOVAngle(), 80.0f);
				FActorSpawnParameters Spawn;
				Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AArenaKillCameraTestPawn* Respawn = Server->SpawnActor<AArenaKillCameraTestPawn>(FVector(200.0, 0.0, 200.0), FRotator::ZeroRotator, Spawn);
				VictimController->Possess(Respawn);
				Stage = 3;
				return false;
			}
			if (Stage == 3)
			{
				if (ClientCamera->IsKillCameraActive())
				{
					return false;
				}
				Test->TestFalse(TEXT("Replicated respawn exits the death view"), ClientCamera->IsKillCameraActive());
				AArenaKillCameraTestPawn* Respawn = Cast<AArenaKillCameraTestPawn>(VictimController->GetPawn());
				ApplyLethalDamage(Respawn, Respawn);
				Stage = 4;
				return false;
			}
			if (ClientCamera->IsKillCameraActive())
			{
				return false;
			}
			Test->TestFalse(TEXT("Replicated suicide preserves the existing death camera"), ClientCamera->IsKillCameraActive());
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TWeakObjectPtr<AArenaPlayerController> VictimController;
		TWeakObjectPtr<AArenaKillCameraTestPawn> OriginalKiller;
		int32 Stage = 0;
	};

	class FRestoreFixture : public IAutomationLatentCommand
	{
	public:
		explicit FRestoreFixture(TSharedRef<FFixture> InFixture) : Fixture(InFixture) {}
		virtual bool Update() override
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE)
				{
					return false;
				}
			}
			Fixture.Reset();
			return true;
		}

	private:
		TSharedPtr<FFixture> Fixture;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaKillCameraNetworkTest, "Arena.Camera.KillCamera.Network.OwnerAndRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaKillCameraNetworkTest::RunTest(const FString& Parameters)
{
	using namespace ArenaKillCameraNetworkTests;
	if (GEditor->PlayWorld)
	{
		AddError(TEXT("Run in an isolated editor without an active PIE session."));
		return false;
	}
	UClass* ControllerClass = LoadClass<AArenaPlayerController>(nullptr, TEXT("/Game/Blueprints/PlayerController/B_ArenaPlayerController.B_ArenaPlayerController_C"));
	if (!TestNotNull(TEXT("Production controller and camera Blueprint"), ControllerClass))
	{
		return false;
	}
	TSharedRef<FFixture> Fixture = MakeShared<FFixture>();
	GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = FSoftClassPath(UGameInstance::StaticClass());
	FNetDriverDefinition Driver;
	Driver.DefName = NAME_GameNetDriver;
	Driver.DriverClassName = TEXT("/Script/OnlineSubsystemUtils.IpNetDriver");
	Driver.DriverClassNameFallback = Driver.DriverClassName;
	GEngine->NetDriverDefinitions.RemoveAll([](const FNetDriverDefinition& Item) { return Item.DefName == NAME_GameNetDriver; });
	GEngine->NetDriverDefinitions.Add(Driver);
	UWorld* EditorWorld = FAutomationEditorCommonUtils::CreateNewMap();
	EditorWorld->GetWorldSettings()->DefaultGameMode = AArenaKillCameraTestGameMode::StaticClass();
	EditorWorld->SpawnActor<APlayerStart>(FVector(0.0, 0.0, 200.0), FRotator::ZeroRotator);
	ULevelEditorPlaySettings* Settings = NewObject<ULevelEditorPlaySettings>();
	Settings->SetPlayNetMode(PIE_ListenServer);
	Settings->SetPlayNumberOfClients(2);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetServerPort(17941);
	Settings->SetClientWindowSize(FIntPoint(640, 480));
	Settings->AddToRoot();
	FRequestPlaySessionParams Request;
	Request.EditorPlaySettings = Settings;
	Request.GameModeOverride = AArenaKillCameraTestGameMode::StaticClass();
	Request.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Request));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckCamera(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreFixture(Fixture));
	return true;
}
#endif
