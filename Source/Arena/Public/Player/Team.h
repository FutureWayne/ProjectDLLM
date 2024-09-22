#pragma once

UENUM(BlueprintType)
enum class ETeam : uint8
{
	ET_Attack UMETA(DisplayName = "Attack"),
	ET_Defense UMETA(DisplayName = "Defense"),
	ET_Neutral UMETA(DisplayName = "Neutral"),
	
	ET_Max UMETA(DisplayName = "DefaultMax")
};