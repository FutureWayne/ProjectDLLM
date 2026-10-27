// Copyright Ludens Studio. All Rights Reserved.

#include "ArenaScoreboardEditorLibrary.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/Widget/ArenaScoreboardStatWidget.h"

bool UArenaScoreboardEditorLibrary::MigrateScoreboardStat(UBlueprint* Blueprint)
{
	if (!Blueprint || Blueprint->ParentClass != UArenaScoreboardStatWidget::StaticClass())
	{
		return false;
	}

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		UEdGraphNode* AsyncNode = nullptr;
		UK2Node_CallFunction* UpdateNode = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node->GetClass()->GetFName() == TEXT("K2Node_AsyncAction_ListenForGameplayMessages"))
			{
				if (AsyncNode)
				{
					return false;
				}
				AsyncNode = Node;
			}
			if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
			{
				if (Call->FunctionReference.GetMemberName() == TEXT("UpdateState"))
				{
					UpdateNode = Call;
				}
			}
		}
		if (!AsyncNode)
		{
			continue;
		}
		if (!UpdateNode || !AsyncNode->FindPin(TEXT("execute")) || !AsyncNode->FindPin(TEXT("PayloadType")))
		{
			return false;
		}
		UScriptStruct* PayloadType = Cast<UScriptStruct>(AsyncNode->FindPin(TEXT("PayloadType"))->DefaultObject);
		if (!PayloadType)
		{
			return false;
		}

		Blueprint->Modify();
		Graph->Modify();
		const TArray<UEdGraphPin*> Inputs = AsyncNode->FindPin(TEXT("execute"))->LinkedTo;
		const int32 NodeX = AsyncNode->NodePosX;
		const int32 NodeY = AsyncNode->NodePosY;
		FBlueprintEditorUtils::RemoveNode(Blueprint, AsyncNode, true);
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		FGraphNodeCreator<UK2Node_CallFunction> EnsureCreator(*Graph);
		UK2Node_CallFunction* Ensure = EnsureCreator.CreateNode();
		Ensure->FunctionReference.SetExternalMember(TEXT("EnsureScoreListener"), UArenaScoreboardStatWidget::StaticClass());
		Ensure->NodePosX = NodeX;
		Ensure->NodePosY = NodeY;
		EnsureCreator.Finalize();
		for (UEdGraphPin* Input : Inputs)
		{
			if (!Schema->TryCreateConnection(Input, Ensure->GetExecPin()))
			{
				return false;
			}
		}
		if (!Schema->TryCreateConnection(Ensure->GetThenPin(), UpdateNode->GetExecPin()))
		{
			return false;
		}

		FGraphNodeCreator<UK2Node_Event> EventCreator(*Graph);
		UK2Node_Event* Event = EventCreator.CreateNode();
		Event->EventReference.SetExternalMember(TEXT("OnScoreChanged"), UArenaScoreboardStatWidget::StaticClass());
		Event->bOverrideFunction = true;
		Event->NodePosX = NodeX;
		Event->NodePosY = NodeY + 320;
		EventCreator.Finalize();
		FGraphNodeCreator<UK2Node_CallFunction> RefreshCreator(*Graph);
		UK2Node_CallFunction* Refresh = RefreshCreator.CreateNode();
		Refresh->FunctionReference = UpdateNode->FunctionReference;
		Refresh->NodePosX = NodeX + 320;
		Refresh->NodePosY = NodeY + 320;
		RefreshCreator.Finalize();
		if (!Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Refresh->GetExecPin()))
		{
			return false;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error)
		{
			return false;
		}
		UArenaScoreboardStatWidget* Defaults = CastChecked<UArenaScoreboardStatWidget>(Blueprint->GeneratedClass->GetDefaultObject());
		Defaults->ScoreMessageChannel = FGameplayTag::RequestGameplayTag(TEXT("Gameplay.Message.ScoreChange"));
		Defaults->ScoreMessagePayloadType = PayloadType;
		Blueprint->MarkPackageDirty();
		return true;
	}
	return false;
}
