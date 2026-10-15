// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"

// Shared between the async task and game-thread lobby update events.
class FSteamLobbySearchState
{
public:
	void SetExpected(TSet<uint64> InExpected)
	{
		FScopeLock Guard(&Lock);
		Expected = MoveTemp(InExpected);
	}
	bool Accept(uint64 LobbyId)
	{
		FScopeLock Guard(&Lock);
		if (bCancelled || !Expected.Contains(LobbyId)) { return false; }
		Received.Add(LobbyId);
		return true;
	}
	bool HasAll() const
	{
		FScopeLock Guard(&Lock);
		return Received.Num() == Expected.Num();
	}
	TArray<uint64> GetReceived() const
	{
		FScopeLock Guard(&Lock);
		return bCancelled ? TArray<uint64>() : Received.Array();
	}
	void Cancel() { FScopeLock Guard(&Lock); bCancelled = true; }
	bool IsCancelled() const { FScopeLock Guard(&Lock); return bCancelled; }
private:
	mutable FCriticalSection Lock;
	bool bCancelled = false;
	TSet<uint64> Expected;
	TSet<uint64> Received;
};
