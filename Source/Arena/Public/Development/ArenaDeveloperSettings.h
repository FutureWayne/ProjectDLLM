// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "ArenaDeveloperSettings.generated.h"


/**
 * Developer settings / editor cheats
 */
UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)
class UArenaDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UArenaDeveloperSettings();

	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface

	// Do the full game flow when playing in the editor, or skip 'waiting for player' / etc... game phases?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=Lyra)
	bool bTestFullGameFlowInPIE = false;
};
