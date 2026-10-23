// Copyright Ludens Studio. All Rights Reserved.

#include "Character/ArenaMeshAssemblyComponent.h"

#include "Character/ArenaHealthComponent.h"
#include "Character/ArenaCharacter.h"
#include "Character/ArenaAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMeshAssemblyLifecycleTest, "Arena.VFX.MeshAssembly.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMeshAssemblyLifecycleTest::RunTest(const FString& Parameters)
{
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Effects/Particles/Impacts/NS_MeshAssembly.NS_MeshAssembly"));
	USkeletalMesh* Asset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Assets/SpiderBot/Meshes/SK_SpiderBot.SK_SpiderBot"));
	UMaterialInterface* Original = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Assets/SpiderBot/Materials/M_SpiderBot.M_SpiderBot"));
	UMaterialInterface* Variant = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Effects/Materials/Assembly/M_SpiderBot_Assembly.M_SpiderBot_Assembly"));
	if (!TestNotNull(TEXT("Shared assembly Niagara"), System) || !TestNotNull(TEXT("Summoned mesh"), Asset) ||
		!TestNotNull(TEXT("Original material"), Original) || !TestNotNull(TEXT("Assembly surface"), Variant))
	{
		return false;
	}

	System->WaitForCompilationComplete(true, false);
	if (!TestTrue(TEXT("Printing Niagara compiles and is ready to run"), System->IsValid() && System->IsReadyToRun()))
	{
		return false;
	}

	for (const TCHAR* Name : {TEXT("PrintLaserFans"), TEXT("PrintSmoke"), TEXT("PrintSparks")})
	{
		const FName StateName(*FString::Printf(TEXT("%s.ExecutionState"), Name));
		TestTrue(FString::Printf(TEXT("%s is connected to the system execution graph"), Name),
			System->GetSystemCompiledData().DataSetCompiledData.Variables.ContainsByPredicate(
				[StateName](const FNiagaraVariable& Variable) { return Variable.GetName() == StateName; }));
	}

	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	for (UClass* CharacterClass : {AArenaCharacter::StaticClass(), AArenaAICharacter::StaticClass()})
	{
		// The first renderable frame must already use the assembly scan, without a timer tick.
		ACharacter* SpawnedCharacter = World->SpawnActorDeferred<ACharacter>(
			CharacterClass, FTransform::Identity);
		USkeletalMeshComponent* SpawnedMesh = SpawnedCharacter->GetMesh();
		SpawnedMesh->SetSkeletalMeshAsset(Asset);
		UMaterialInstanceDynamic* SpawnMaterial = UMaterialInstanceDynamic::Create(Original, SpawnedCharacter);
		SpawnedMesh->SetMaterial(0, SpawnMaterial);
		UArenaMeshAssemblyComponent* SpawnAssembly = NewObject<UArenaMeshAssemblyComponent>(SpawnedCharacter);
		SpawnAssembly->AssemblySystem = System;
		SpawnAssembly->ScanMaterials.Add(Original, Variant);
		SpawnedCharacter->AddInstanceComponent(SpawnAssembly);
		SpawnAssembly->RegisterComponent();
		SpawnedCharacter->FinishSpawning(FTransform::Identity);
		// The standalone test world has no match GameState to dispatch BeginPlay automatically.
		if (!SpawnedCharacter->HasActorBegunPlay())
		{
			SpawnedCharacter->DispatchBeginPlay();
		}
		TestTrue(TEXT("Character or summoned creature assembles before any timer tick"), SpawnAssembly->IsAssembling());
		UMaterialInstanceDynamic* SpawnScan = Cast<UMaterialInstanceDynamic>(SpawnedMesh->GetMaterial(0));
		if (TestTrue(TEXT("First frame already has the scan material"), SpawnScan && SpawnScan != SpawnMaterial))
		{
			TestEqual(TEXT("First frame starts with the model unrevealed"), SpawnScan->K2_GetScalarParameterValue(TEXT("AssemblyProgress")), 0.0f);
			SpawnAssembly->TickComponent(SpawnAssembly->Duration * 0.25f, LEVELTICK_All, nullptr);
			World->GetTimerManager().Tick(0.016f);
			TestEqual(TEXT("Fallback timer does not replace the first-frame material"), SpawnedMesh->GetMaterial(0), static_cast<UMaterialInterface*>(SpawnScan));
			TestEqual(TEXT("Fallback timer does not restart assembly"), SpawnScan->K2_GetScalarParameterValue(TEXT("AssemblyProgress")), 0.25f);
		}
		SpawnAssembly->StopAssembly();
		TestEqual(TEXT("Spawn initialization preserves the original team material"), SpawnedMesh->GetMaterial(0), static_cast<UMaterialInterface*>(SpawnMaterial));
		SpawnedCharacter->Destroy();
	}

	AActor* Owner = World->SpawnActor<AActor>();
	USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(Owner);
	Owner->SetRootComponent(Mesh);
	Mesh->SetSkeletalMeshAsset(Asset);
	Mesh->SetMaterial(0, Original);
	Mesh->RegisterComponent();
	UArenaMeshAssemblyComponent* Assembly = NewObject<UArenaMeshAssemblyComponent>(Owner);
	Assembly->bAutoPlay = false;
	Assembly->AssemblySystem = System;
	Assembly->ScanMaterials.Add(Original, Variant);
	Assembly->RegisterComponent();

	Assembly->PlayAssembly(Mesh);
	TestTrue(TEXT("Assembly starts"), Assembly->IsAssembling());
	UMaterialInstanceDynamic* Scan = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
	TestNotNull(TEXT("Original surface has a temporary dynamic scan material"), Scan);
	Assembly->TickComponent(Assembly->Duration * 0.5f, LEVELTICK_All, nullptr);
	if (Scan)
	{
		TestEqual(TEXT("Scan follows shared progress"), Scan->K2_GetScalarParameterValue(TEXT("AssemblyProgress")), 0.5f);
		TestEqual(TEXT("Print edge uses the configured laser color"), Scan->K2_GetVectorParameterValue(TEXT("AssemblyColor")), Assembly->PrintingColor);
	}
	Assembly->TickComponent(Assembly->Duration, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Assembly finishes"), Assembly->IsAssembling());
	TestFalse(TEXT("No idle tick"), Assembly->IsComponentTickEnabled());
	TestEqual(TEXT("Original material restored"), Mesh->GetMaterial(0), Original);
	TArray<UNiagaraComponent*> Effects;
	Owner->GetComponents(Effects);
	TestEqual(TEXT("Temporary effects cleaned up"), Effects.Num(), 0);

	UMaterialInstanceDynamic* TeamMaterial = UMaterialInstanceDynamic::Create(Original, Owner);
	Mesh->SetMaterial(0, TeamMaterial);
	Assembly->PlayAssembly(Mesh);
	TestTrue(TEXT("Existing team MID is supported"), Assembly->IsAssembling());
	Assembly->StopAssembly();
	TestEqual(TEXT("Existing team MID restored by identity"), Mesh->GetMaterial(0), static_cast<UMaterialInterface*>(TeamMaterial));

	Assembly->PlayAssembly(Mesh);
	UMaterialInstanceDynamic* Replacement = UMaterialInstanceDynamic::Create(Original, Owner);
	Mesh->SetMaterial(0, Replacement);
	Assembly->StopAssembly();
	TestEqual(TEXT("External material replacement is preserved"), Mesh->GetMaterial(0), static_cast<UMaterialInterface*>(Replacement));
	Mesh->SetMaterial(0, Original);

	Assembly->Duration = 0.0f;
	Assembly->PlayAssembly(Mesh);
	TestFalse(TEXT("Invalid duration does not hide the model"), Assembly->IsAssembling());
	Assembly->Duration = 0.55f;
	Owner->SetActorHiddenInGame(true);
	Assembly->PlayAssembly(Mesh);
	TestFalse(TEXT("Hidden actors do not assemble"), Assembly->IsAssembling());
	Owner->SetActorHiddenInGame(false);

	UArenaHealthComponent* Health = NewObject<UArenaHealthComponent>(Owner);
	Health->RegisterComponent();
	Assembly->PlayAssembly(Mesh);
	Health->StartDeath();
	Assembly->TickComponent(0.016f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Death interrupts assembly"), Assembly->IsAssembling());
	TestEqual(TEXT("Death receives the original material"), Mesh->GetMaterial(0), Original);
	Assembly->PlayAssembly(Mesh);
	TestFalse(TEXT("A dying actor cannot restart assembly"), Assembly->IsAssembling());

	Owner->Destroy();

	// The production hero mesh has an unassigned material slot; it must not suppress the whole effect.
	AActor* HeroOwner = World->SpawnActor<AActor>();
	USkeletalMeshComponent* HeroMesh = NewObject<USkeletalMeshComponent>(HeroOwner);
	HeroOwner->SetRootComponent(HeroMesh);
	HeroMesh->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/CyberNinja/Meshes/SK_CyberNinja.SK_CyberNinja")));
	HeroMesh->RegisterComponent();
	UArenaMeshAssemblyComponent* HeroAssembly = NewObject<UArenaMeshAssemblyComponent>(HeroOwner);
	HeroAssembly->bAutoPlay = false;
	HeroAssembly->AssemblySystem = System;
	for (const TCHAR* Name : {TEXT("M_Legs"), TEXT("M_Body"), TEXT("M_Head")})
	{
		UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr,
			*FString::Printf(TEXT("/Game/Characters/CyberNinja/Materials/%s.%s"), Name, Name));
		UMaterialInterface* ScanMaterial = LoadObject<UMaterialInterface>(nullptr,
			*FString::Printf(TEXT("/Game/Effects/Materials/Assembly/%s_Assembly.%s_Assembly"), Name, Name));
		HeroAssembly->ScanMaterials.Add(Source, ScanMaterial);
	}
	HeroAssembly->ScanMaterials.Add(LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial")),
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Effects/Materials/Assembly/WorldGridMaterial_Assembly.WorldGridMaterial_Assembly")));
	HeroAssembly->RegisterComponent();
	HeroAssembly->PlayAssembly(HeroMesh);
	TestTrue(TEXT("Production hero assembles with an unassigned material slot"), HeroAssembly->IsAssembling());
	HeroAssembly->StopAssembly();
	HeroOwner->Destroy();
	Instance->Shutdown();
	return true;
}
#endif
