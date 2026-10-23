#include "UI/Widget/ArenaTopBarWidget.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "Teams/ArenaTeamInfo.h"
#include "UObject/UnrealType.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaTopBarSnapshotTest,"Arena.UI.TopBar.Snapshot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FArenaTopBarSnapshotTest::RunTest(const FString& Parameters)
{
 UGameInstance* Instance=NewObject<UGameInstance>(GEngine); Instance->InitializeStandalone();
 UWorld* World=Instance->GetWorld(); World->InitializeActorsForPlay(FURL()); World->BeginPlay();
 AGameStateBase* State=World->SpawnActor<AGameStateBase>(); World->SetGameState(State);
 APlayerController* Controller=World->SpawnActor<APlayerController>();
 ULocalPlayer* Player=NewObject<ULocalPlayer>(GEngine); Instance->AddLocalPlayer(Player,FPlatformUserId::CreateFromInternalId(0)); Controller->SetPlayer(Player);
 UClass* ScoringClass=LoadClass<UActorComponent>(nullptr,TEXT("/Game/Blueprints/Game/GameState/B_ArenaDeathMatchScoring.B_ArenaDeathMatchScoring_C"));
 if(!TestNotNull(TEXT("Real TDM scoring class"),ScoringClass)) return false;
 UActorComponent* Source=NewObject<UActorComponent>(State,ScoringClass); State->AddOwnedComponent(Source);
 auto Set=[&](FName Name,double Value) {
  FNumericProperty* P=FindFProperty<FNumericProperty>(ScoringClass,Name);
  if(!TestNotNull(*Name.ToString(),P))return;
  void* Data=P->ContainerPtrToValuePtr<void>(Source);
  if(P->IsFloatingPoint())P->SetFloatingPointPropertyValue(Data,Value); else P->SetIntPropertyValue(Data,int64(Value));
 };
 UArenaTeamSubsystem* Teams=World->GetSubsystem<UArenaTeamSubsystem>();
 const FGameplayTag ScoreTag=FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"));
 for(int32 TeamId : {1,2}) {
  AArenaTeamInfo* Team=World->SpawnActor<AArenaTeamInfo>();
  FindFProperty<FIntProperty>(Team->GetClass(),TEXT("TeamId"))->SetPropertyValue_InContainer(Team,TeamId);
  Teams->RegisterTeamInfo(Team);
 }
 UArenaTopBarWidget* Widget=CreateWidget<UArenaTopBarWidget>(Controller); auto Slate=Widget->TakeWidget();
 Widget->RefreshSnapshot(); TestFalse(TEXT("Zero/unconfigured target stays syncing"),Widget->bHasData);
 Set(TEXT("TargetScore"),75); Set(TEXT("DefenderScore"),0);Set(TEXT("AttackerScore"),0);
 Teams->AddTeamTagStack(2,ScoreTag,24);Teams->AddTeamTagStack(1,ScoreTag,18);Set(TEXT("CountdownTime"),148);
 Widget->RefreshSnapshot();
 TestTrue(TEXT("Reads valid replicated snapshot"),Widget->bHasData);
 TestEqual(TEXT("Uses configured target rather than reference 50"),Widget->TargetKills,75);
 TestEqual(TEXT("Blue kills despite stale zero Blueprint cache"),Widget->Kills[0],24);TestEqual(TEXT("Red kills despite stale zero Blueprint cache"),Widget->Kills[1],18);
 TestEqual(TEXT("Countdown"),Widget->Seconds,148);TestEqual(TEXT("Join has no false kill pulse"),Widget->Pulse[0],0.f);
 TestTrue(TEXT("Normalized using configured goal"),FMath::IsNearlyEqual(Widget->Progress[0],24.f/75));
 auto Capture=[&](const FString& Name){
  if(!FApp::CanEverRender())return;
  FWidgetRenderer Renderer(true,false); UTextureRenderTarget2D* RT=Renderer.DrawWidget(Slate,FVector2D(1600,112));
  if(RT){FBufferArchive PNG; if(FImageUtils::ExportRenderTarget2DAsPNG(RT,PNG)) FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("topbar-")+Name+TEXT(".png")));}
 };
 Capture(TEXT("normal"));
 Teams->AddTeamTagStack(2,ScoreTag,1); Widget->RefreshSnapshot();TestEqual(TEXT("Kill pulse starts"),Widget->Pulse[0],1.f);
 Widget->NativeTick(FGeometry(),.05f);TestTrue(TEXT("Progress chases new score"),Widget->Progress[0]>24.f/75 && Widget->Progress[0]<25.f/75);
 Set(TEXT("CountdownTime"),9);Widget->NativeTick(FGeometry(),.06f);Capture(TEXT("warning"));
 Teams->AddTeamTagStack(2,ScoreTag,75);Widget->RefreshSnapshot();for(int I=0;I<90;++I)Widget->NativeTick(FGeometry(),.016f);
 TestTrue(TEXT("Overtime score fill clamped"),Widget->Progress[0]<=1.f);
 Teams->AddTeamTagStack(1,ScoreTag,1);Widget->RefreshSnapshot();TestEqual(TEXT("Red independent increment"),Widget->Kills[1],19);
 Teams->RemoveTeamTagStack(2,ScoreTag,100);Widget->RefreshSnapshot();TestEqual(TEXT("Round reset clears blue score"),Widget->Kills[0],0);
 Set(TEXT("CountdownTime"),-4);Widget->RefreshSnapshot();TestEqual(TEXT("Time never negative"),Widget->Seconds,0);
 State->RemoveOwnedComponent(Source);Widget->ScoringSource.Reset();Widget->RefreshSnapshot();TestFalse(TEXT("Missing source invalidates snapshot"),Widget->bHasData);
 Widget->NativeDestruct(); Instance->Shutdown(); World->DestroyWorld(false); return true;
}
#endif



