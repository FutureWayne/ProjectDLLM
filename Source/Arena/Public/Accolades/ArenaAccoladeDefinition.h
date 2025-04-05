// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "ArenaAccoladeDefinition.generated.h"

class UObject;
class USoundBase;

USTRUCT(BlueprintType)
struct FArenaAccoladeDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// The message to display
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	// The sound to play
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundBase> Sound;

	// The icon to display	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail="true", AllowedClasses="Texture,MaterialInterface,SlateTextureAtlasInterface", DisallowedClasses="MediaTexture"))
	TSoftObjectPtr<UObject> Icon;

	// Duration (in seconds) to display this accolade
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DisplayDuration = 1.0f;

	// Location to display this accolade
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag LocationTag;

	// Tags associated with this accolade
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer AccoladeTags;

	// When this accolade is displayed, any existing displayed/pending accolades with any of
	// these tags will be removed (e.g., getting a triple-elim will suppress a double-elim)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer CancelAccoladesWithTag;
};
