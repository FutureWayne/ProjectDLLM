// Copyright Ludens Studio. All Rights Reserved.

#include "ArenaGroundAreaVfxEditorLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraphPin.h"
#include "Modules/ModuleManager.h"
#include "INiagaraEditorTypeUtilities.h"
#include "NiagaraEditorModule.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraSystem.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"

namespace
{
	void SetFloatInput(UNiagaraNodeFunctionCall& Function, const TCHAR* InputName, float Value)
	{
		const FString Name = Function.GetFunctionName() + TEXT(".") + InputName;
		UEdGraphPin& Pin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			Function, FNiagaraParameterHandle(*Name), FNiagaraTypeDefinition::GetFloatDef(), FGuid(), FGuid());
		Pin.DefaultValue = FString::SanitizeFloat(Value);
	}

	bool ConfigureDropMotion(FVersionedNiagaraEmitterData& Data)
	{
		UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Data.GraphSource);
		UNiagaraGraph* Graph = Source ? Source->NodeGraph : nullptr;
		UNiagaraScript* CurlScript = LoadObject<UNiagaraScript>(nullptr,
			TEXT("/Niagara/Modules/Update/Forces/V2/CurlNoiseForce.CurlNoiseForce"));
		UNiagaraScript* RandomScript = LoadObject<UNiagaraScript>(nullptr,
			TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedFloat.UniformRangedFloat"));
		UNiagaraNodeOutput* Output = Graph ? Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript) : nullptr;
		if (!Graph || !CurlScript || !RandomScript || !Output)
		{
			return false;
		}

		UNiagaraNodeFunctionCall* Curl = nullptr;
		UNiagaraNodeFunctionCall* Initialize = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Node);
			if (!Function)
			{
				continue;
			}

			if (Function->FunctionScript == CurlScript)
			{
				Curl = Function;
			}
			else if (Function->GetFunctionName() == TEXT("InitializeParticle001"))
			{
				Initialize = Function;
			}
		}

		if (!Initialize)
		{
			return false;
		}

		if (!Curl)
		{
			TArray<UNiagaraNodeFunctionCall*> Modules;
			UEdGraphNode* Previous = Output;
			while (Previous)
			{
				UEdGraphPin** Input = Previous->Pins.FindByPredicate([](const UEdGraphPin* Pin)
				{
					return Pin->Direction == EGPD_Input && Pin->PinType.PinSubCategoryObject == FNiagaraTypeDefinition::GetParameterMapDef().GetStruct();
				});
				if (!Input || (*Input)->LinkedTo.Num() != 1)
				{
					break;
				}

				Previous = (*Input)->LinkedTo[0]->GetOwningNode();
				if (UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Previous))
				{
					Modules.Insert(Function, 0);
				}
			}
			const int32 SolverIndex = Modules.IndexOfByPredicate([](const UNiagaraNodeFunctionCall* Function)
			{
				return Function->FunctionScript && Function->FunctionScript->GetName() == TEXT("SolveForcesAndVelocity");
			});
			if (SolverIndex == INDEX_NONE)
			{
				return false;
			}

			Curl = FNiagaraStackGraphUtilities::AddScriptModuleToStack(CurlScript, *Output, SolverIndex, TEXT("FallingFireTurbulence"));
		}

		if (!Curl)
		{
			return false;
		}

		SetFloatInput(*Curl, TEXT("Noise Strength"), 90.f);
		SetFloatInput(*Curl, TEXT("Noise Frequency"), 12.f);
		UEdGraphPin& RandomPin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			*Initialize, FNiagaraParameterHandle(TEXT("InitializeParticle001.Material Random")), FNiagaraTypeDefinition::GetFloatDef(), FGuid(), FGuid());
		UNiagaraNodeFunctionCall* Random = RandomPin.LinkedTo.Num() == 1
			? Cast<UNiagaraNodeFunctionCall>(RandomPin.LinkedTo[0]->GetOwningNode())
			: nullptr;
		if (!Random || Random->FunctionScript != RandomScript)
		{
			RandomPin.BreakAllPinLinks();
			FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(RandomPin, RandomScript, Random);
		}

		if (!Random)
		{
			return false;
		}

		SetFloatInput(*Random, TEXT("Minimum"), 0.f);
		SetFloatInput(*Random, TEXT("Maximum"), 1.f);
		Graph->NotifyGraphChanged();
		return true;
	}

	bool ConfigureDropVelocity(FVersionedNiagaraEmitterData& Data)
	{
		UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Data.GraphSource);
		UNiagaraGraph* Graph = Source ? Source->NodeGraph : nullptr;
		UNiagaraScript* RandomVelocity = LoadObject<UNiagaraScript>(nullptr,
			TEXT("/Niagara/DynamicInputs/UniformRange/UniformRangedVector.UniformRangedVector"));
		if (!Graph || !RandomVelocity)
		{
			return false;
		}

		UNiagaraNodeFunctionCall* AddVelocity = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UNiagaraNodeFunctionCall* Function = Cast<UNiagaraNodeFunctionCall>(Node);
			if (!Function)
			{
				continue;
			}

			if (Function->GetFunctionName() == TEXT("AddVelocity001"))
			{
				AddVelocity = Function;
			}
		}

		if (!AddVelocity)
		{
			return false;
		}

		UEdGraphPin& VelocityPin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			*AddVelocity, FNiagaraParameterHandle(TEXT("AddVelocity001.Velocity")), FNiagaraTypeDefinition::GetVec3Def(), FGuid(), FGuid());
		UNiagaraNodeFunctionCall* DropVelocity = VelocityPin.LinkedTo.Num() == 1
			? Cast<UNiagaraNodeFunctionCall>(VelocityPin.LinkedTo[0]->GetOwningNode())
			: nullptr;
		if (!DropVelocity || DropVelocity->FunctionScript != RandomVelocity)
		{
			VelocityPin.BreakAllPinLinks();
			FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(VelocityPin, RandomVelocity, DropVelocity);
		}

		FNiagaraEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
		TSharedPtr<INiagaraEditorTypeUtilities, ESPMode::ThreadSafe> TypeUtilities = EditorModule.GetTypeUtilities(FNiagaraTypeDefinition::GetVec3Def());
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const FString InputName = DropVelocity->GetFunctionName() + (Index == 0 ? TEXT(".Minimum") : TEXT(".Maximum"));
			UEdGraphPin& InputPin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
				*DropVelocity, FNiagaraParameterHandle(*InputName), FNiagaraTypeDefinition::GetVec3Def(), FGuid(), FGuid());
			FNiagaraVariable Value(FNiagaraTypeDefinition::GetVec3Def(), NAME_None);
			Value.SetValue(Index == 0 ? FVector3f(-60.f, -60.f, -320.f) : FVector3f(60.f, 60.f, -80.f));
			InputPin.DefaultValue = TypeUtilities->GetPinDefaultStringFromValue(Value);
		}

		Graph->NotifyGraphChanged();
		return true;
	}
}

UNiagaraSystem* UArenaGroundAreaVfxEditorLibrary::CreateFallingFireSystem(float FlameDensity)
{
	const TCHAR* Path = TEXT("/Game/Effects/Particles/NS_Explosion/NS_IncendiaryFallingFire");
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/Effects/Particles/NS_Explosion/NS_IncendiaryFallingFire.NS_IncendiaryFallingFire"), nullptr, LOAD_NoWarn);
	const bool bNewAsset = System == nullptr;
	if (bNewAsset)
	{
		UNiagaraSystem* Source = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Effects/Particles/NS_Explosion/NS_IncendiaryGroundFire.NS_IncendiaryGroundFire"));
		if (!Source)
		{
			return nullptr;
		}

		System = CastChecked<UNiagaraSystem>(StaticDuplicateObject(Source, CreatePackage(Path), TEXT("NS_IncendiaryFallingFire")));
		System->SetFlags(RF_Public | RF_Standalone);
	}

	System->Modify();
	System->GetExposedParameters().SetParameterValue(FMath::Max(0.f, FlameDensity),
		FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.FlamesEnableWisps")));
	int32 RadiusInputs = 0;
	for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* Data = Handle.GetEmitterData();
		if (!Data)
		{
			continue;
		}

		if (Handle.GetName() != TEXT("Fire_Licks"))
		{
			continue;
		}

		// World-space particles retain their own trajectories as the emission source falls.
		Data->bLocalSpace = false;
		const bool bVelocityConfigured = ConfigureDropVelocity(*Data);
		const bool bMotionConfigured = ConfigureDropMotion(*Data);
		if (!ensure(bVelocityConfigured && bMotionConfigured))
		{
			return nullptr;
		}

		TArray<UNiagaraScript*> Scripts;
		Data->GetScripts(Scripts);
		for (UNiagaraScript* Script : Scripts)
		{
			if (!Script)
			{
				continue;
			}

			TArray<FNiagaraVariable> Parameters;
			Script->RapidIterationParameters.GetParameters(Parameters);
			for (const FNiagaraVariable& Parameter : Parameters)
			{
				const FString Name = Parameter.GetName().ToString();
				if (Parameter.GetType() == FNiagaraTypeDefinition::GetFloatDef())
				{
					float Value = Script->RapidIterationParameters.GetParameterValue<float>(Parameter);
					if (Name.EndsWith(TEXT(".Multiply_Float002.A")))
					{
						Value = 450.f;
						++RadiusInputs;
					}
					else if (Name.EndsWith(TEXT(".ShapeLocation.Cylinder Height")))
					{
						Value = 800.f;
					}
					else if (Name.EndsWith(TEXT(".Multiply_Float005.A")))
					{
						Value = 60.f;
					}
					else if (Name.EndsWith(TEXT(".Multiply_Float006.A")))
					{
						Value = 120.f;
					}
					else if (Name.EndsWith(TEXT(".InitializeParticle001.Lifetime Min")))
					{
						Value = 1.1f;
					}
					else if (Name.EndsWith(TEXT(".InitializeParticle001.Lifetime Max")))
					{
						Value = 1.8f;
					}

					Script->RapidIterationParameters.SetParameterValue(Value, Parameter);
				}
				else if (Parameter.GetType() == FNiagaraTypeDefinition::GetVec3Def())
				{
					if (Name.EndsWith(TEXT(".ShapeLocation.Non Uniform Scale")))
					{
						Script->RapidIterationParameters.SetParameterValue(FVector3f(1.f), Parameter);
					}
					else if (Name.EndsWith(TEXT(".UniformRangedVector.Minimum")))
					{
						Script->RapidIterationParameters.SetParameterValue(FVector3f(-60.f, -60.f, -320.f), Parameter);
					}
					else if (Name.EndsWith(TEXT(".UniformRangedVector.Maximum")))
					{
						Script->RapidIterationParameters.SetParameterValue(FVector3f(60.f, 60.f, -80.f), Parameter);
					}
				}
			}
		}
	}

	if (!ensure(RadiusInputs > 0))
	{
		return nullptr;
	}

	if (bNewAsset)
	{
		FAssetRegistryModule::AssetCreated(System);
	}
	System->RequestCompile(true);
	System->WaitForCompilationComplete(true, false);
	System->MarkPackageDirty();
	return System;
}
