// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Actor/ArenaGroundAreaEffect.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "Misc/AutomationTest.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
	struct FFireNetworkFixture
	{
		TArray<FNetDriverDefinition> SavedDrivers = GEngine->NetDriverDefinitions;
		FSoftClassPath SavedGameInstance = GetDefault<UGameMapsSettings>()->GameInstanceClass;
		TWeakObjectPtr<AArenaGroundAreaEffect> ServerField;
		TWeakObjectPtr<AArenaGroundAreaEffect> ClientField;

		~FFireNetworkFixture()
		{
			GEngine->NetDriverDefinitions = SavedDrivers;
			GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = SavedGameInstance;
		}
	};

	class FCheckFireReplication : public IAutomationLatentCommand
	{
	public:
		FCheckFireReplication(FAutomationTestBase* InTest, TSharedRef<FFireNetworkFixture> InFixture)
			: Test(InTest)
			, Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > 30.)
			{
				Test->AddError(FString::Printf(TEXT("Fire replication timed out in stage %d"), Stage));
				return true;
			}

			UWorld* Server = nullptr;
			UWorld* Client = nullptr;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType != EWorldType::PIE || !Context.World())
				{
					continue;
				}

				if (Context.World()->GetNetMode() == NM_ListenServer)
				{
					Server = Context.World();
				}

				if (Context.World()->GetNetMode() == NM_Client)
				{
					Client = Context.World();
				}
			}

			if (!Server || !Client || !Client->HasBegunPlay())
			{
				return false;
			}

			if (Stage == 0)
			{
				FArenaGroundAreaSettings Settings;
				Settings.Radius = 120.f;
				Settings.CellsPerFrame = 1;
				Settings.Lifetime = 4.f;
				Settings.DestroyDelay = 1.5f;
				Settings.MaxVisualCount = 2;
				Settings.bFallToGround = true;
				Settings.FallAcceleration = 500.f;
				Settings.TerminalFallSpeed = 1000.f;
				const FTransform Transform(FRotator(0.f, 37.f, 0.f), FVector(0, 0, 1000));
				AArenaGroundAreaEffect* Fire = Server->SpawnActorDeferred<AArenaGroundAreaEffect>(AArenaGroundAreaEffect::StaticClass(), Transform);
				Fire->Initialize(Settings);
				Fire->FinishSpawning(Transform);
				Fixture->ServerField = Fire;
				Stage = 1;
				return false;
			}

			if (Stage == 1)
			{
				for (TActorIterator<AArenaGroundAreaEffect> It(Client); It; ++It)
				{
					Fixture->ClientField = *It;
				}

				AArenaGroundAreaEffect* Fire = Fixture->ServerField.Get();
				AArenaGroundAreaEffect* Copy = Fixture->ClientField.Get();
				if (!Fire || !Copy || !Copy->IsFalling() || Copy->GetActorLocation().Z >= 999.)
				{
					return false;
				}

				Test->TestTrue(TEXT("Authority and remote both enter the falling stage"), Fire->IsFalling() && Copy->IsFalling());
				Test->TestTrue(TEXT("Remote falling location is between explosion and ground"), Copy->GetActorLocation().Z > 2. && Copy->GetActorLocation().Z < 1000.);
				Test->TestTrue(TEXT("Server and client use the same falling clock"), FMath::Abs(Fire->GetActorLocation().Z - Copy->GetActorLocation().Z) < 150.);
				Test->TestTrue(TEXT("No remote ground fire appears while falling"), Copy->GetVisualCells().IsEmpty() && Fire->GetVisualCells().IsEmpty());
				Stage = 2;
				return false;
			}

			if (Stage == 2)
			{
				for (TActorIterator<AArenaGroundAreaEffect> It(Client); It; ++It)
				{
					Fixture->ClientField = *It;
				}

				AArenaGroundAreaEffect* Fire = Fixture->ServerField.Get();
				AArenaGroundAreaEffect* Copy = Fixture->ClientField.Get();
				if (!Fire || !Copy || !Fire->IsDiffusionComplete() || Copy->GetVisualCells().IsEmpty())
				{
					return false;
				}

				Test->TestFalse(TEXT("Remote field has no authority"), Copy->HasAuthority());
				Test->TestTrue(TEXT("Client never computes logical cells"), Copy->GetCells().IsEmpty());
				Test->TestFalse(TEXT("Client never runs diffusion completion"), Copy->IsDiffusionComplete());
				Test->TestEqual(TEXT("Remote merged-cell count matches authority"), Copy->GetVisualCells().Num(), Fire->GetVisualCells().Num());
				Test->TestTrue(TEXT("Projected ground location reaches client"), FMath::Abs(Copy->GetActorLocation().Z - 2.) < 1.);
				Test->TestEqual(TEXT("Visual cap reaches remote client"), Copy->GetSettings().MaxVisualCount, 2);
				Test->TestTrue(TEXT("Initial yaw survives network spawn"), FMath::IsNearlyEqual(Copy->GetActorRotation().Yaw, 37., 0.1));
				for (int32 I = 0; I < FMath::Min(Copy->GetVisualCells().Num(), Fire->GetVisualCells().Num()); ++I)
				{
					const FArenaGroundVisualCell& A = Fire->GetVisualCells()[I];
					const FArenaGroundVisualCell& B = Copy->GetVisualCells()[I];
					Test->TestEqual(TEXT("Merged index is replicated"), B.Index, A.Index);
					Test->TestTrue(TEXT("Representative location is replicated"), FVector(B.Location).Equals(A.Location, 0.2f));
					Test->TestTrue(TEXT("Display height is replicated"), FMath::IsNearlyEqual(A.MaxHeight, B.MaxHeight, 0.01f));
				}

				Test->TestFalse(TEXT("Landing transition reaches the client"), Copy->IsFalling());
				Test->TestNull(TEXT("Landing clears the falling effect"), Copy->GetFallingVisual());
				Stage = 3;
				return false;
			}

			if (Stage == 3)
			{
				const AArenaGroundAreaEffect* Copy = Fixture->ClientField.Get();
				if (!Copy || !Copy->GetVisualCells().IsEmpty())
				{
					return false;
				}

				Test->TestTrue(TEXT("Lifetime expiry clears remote visuals"), Copy->GetVisualCells().IsEmpty());
				Test->TestTrue(TEXT("Lifetime expiry never starts client-side physics diffusion"), Copy->GetCells().IsEmpty());
				Stage = 4;
				return false;
			}

			if (Fixture->ServerField.IsValid() || Fixture->ClientField.IsValid())
			{
				return false;
			}

			Test->TestTrue(TEXT("Field destruction is replicated after lifetime and destroy delay"), true);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FFireNetworkFixture> Fixture;
		int32 Stage = 0;
	};

	class FRestoreFireNetworkFixture : public IAutomationLatentCommand
	{
	public:
		explicit FRestoreFireNetworkFixture(TSharedRef<FFireNetworkFixture> InFixture)
			: Fixture(InFixture)
		{
		}

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
		TSharedPtr<FFireNetworkFixture> Fixture;
	};

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireNetworkTest, "Arena.Incendiary.Network.AuthorityAndLifetimeReplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFireNetworkTest::RunTest(const FString&)
{
	if (GEditor->PlayWorld)
	{
		AddError(TEXT("Run this regression in an isolated editor with no active PIE session."));
		return false;
	}

	TSharedRef<FFireNetworkFixture> Fixture = MakeShared<FFireNetworkFixture>();
	GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = FSoftClassPath(UGameInstance::StaticClass());
	FNetDriverDefinition Driver;
	Driver.DefName = NAME_GameNetDriver;
	Driver.DriverClassName = TEXT("/Script/OnlineSubsystemUtils.IpNetDriver");
	Driver.DriverClassNameFallback = Driver.DriverClassName;
	GEngine->NetDriverDefinitions.RemoveAll([](const FNetDriverDefinition& Item)
		{
			return Item.DefName == NAME_GameNetDriver;
		});
	GEngine->NetDriverDefinitions.Add(Driver);
	UWorld* EditorWorld = FAutomationEditorCommonUtils::CreateNewMap();
	EditorWorld->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	AStaticMeshActor* Floor = EditorWorld->SpawnActor<AStaticMeshActor>();
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(20, 20, 1));
	Floor->SetActorLocation(FVector(0, 0, -50));
	EditorWorld->SpawnActor<APlayerStart>(FVector(0, 0, 200), FRotator::ZeroRotator);
	ULevelEditorPlaySettings* PlaySettings = NewObject<ULevelEditorPlaySettings>();
	PlaySettings->SetPlayNetMode(PIE_ListenServer);
	PlaySettings->SetPlayNumberOfClients(2);
	PlaySettings->SetRunUnderOneProcess(true);
	PlaySettings->SetServerPort(17937);
	PlaySettings->SetClientWindowSize(FIntPoint(320, 240));
	PlaySettings->AddToRoot(); // Balanced by FStartPIEForAutomationCommand.
	FRequestPlaySessionParams Params;
	Params.EditorPlaySettings = PlaySettings;
	Params.GameModeOverride = AGameModeBase::StaticClass();
	Params.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckFireReplication(this, Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreFireNetworkFixture(Fixture));
	return true;
}
#endif
