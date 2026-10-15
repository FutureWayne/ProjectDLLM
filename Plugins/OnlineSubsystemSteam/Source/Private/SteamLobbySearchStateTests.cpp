// Copyright Epic Games, Inc. All Rights Reserved.
#include "SteamLobbySearchState.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSteamLateLobbyDataTest, "Online.Steam.Session.LateLobbyDataIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSteamLateLobbyDataTest::RunTest(const FString& Parameters)
{
	FSteamLobbySearchState OldQuery;
	OldQuery.SetExpected(TSet<uint64>{101, 102});
	OldQuery.Accept(101);
	OldQuery.Cancel();
	FSteamLobbySearchState Retry;
	// Updates arriving before the replacement has its own result list are ignored.
	TestFalse(TEXT("Old lobby data cannot enter an uninitialized retry"), Retry.Accept(102));
	Retry.SetExpected(TSet<uint64>{201, 202});
	TestFalse(TEXT("Late old LobbyDataUpdate is rejected by retry"), Retry.Accept(102));
	TestFalse(TEXT("Cancelled task cannot accept more lobby data"), OldQuery.Accept(102));
	TestTrue(TEXT("Cancelled result snapshot is empty"), OldQuery.GetReceived().IsEmpty());
	TestTrue(TEXT("Retry accepts its first expected lobby"), Retry.Accept(201));
	Retry.Accept(201);
	TestFalse(TEXT("Duplicate updates cannot complete retry early"), Retry.HasAll());
	TestEqual(TEXT("Only the expected unique lobby is collected"), Retry.GetReceived().Num(), 1);
	Retry.Accept(202);
	TestTrue(TEXT("Retry completes only when its own lobby set arrives"), Retry.HasAll());
	TestEqual(TEXT("Retry results exclude old lobbies"), Retry.GetReceived().Num(), 2);
	TestFalse(TEXT("Stale lobby cannot alter completed retry results"), Retry.Accept(101));
	TestEqual(TEXT("Old query remains cancelled after retry completes"), OldQuery.GetReceived().Num(), 0);
	return true;
}
#endif
