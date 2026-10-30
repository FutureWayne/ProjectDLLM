// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "DataRegistrySource_DataTable.h"
#include "ArenaAccoladeHostTestTypes.generated.h"

/** Controls when the real table source resolves requests, including an asynchronously missing row. */
UCLASS(Transient)
class UArenaAccoladeTestTableSource : public UDataRegistrySource_DataTable
{
	GENERATED_BODY()

public:
	virtual EDataRegistryAvailability GetItemAvailability(const FName& ResolvedName, const uint8** PrecachedDataPtr) const override;
	bool CompleteRequest(FName RowName);
	int32 GetPendingCount() const { return PendingAcquires.Num(); }

protected:
	virtual void HandlePendingAcquires() override;
};
