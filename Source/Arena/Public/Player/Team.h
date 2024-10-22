#pragma once

UENUM(BlueprintType)
enum class ETeam : uint8
{
	ET_Neutral UMETA(DisplayName = "Neutral"),
	ET_Attack UMETA(DisplayName = "Attack"),
	ET_Defense UMETA(DisplayName = "Defense"),
	
	ET_Max UMETA(DisplayName = "DefaultMax")
};

inline FLinearColor GetTeamColor(ETeam Team)
{
	switch (Team)
	{
	case ETeam::ET_Neutral:
		return FLinearColor::White;
	case ETeam::ET_Attack:
		return FLinearColor::Red;
	case ETeam::ET_Defense:
		return FLinearColor::Blue;
	default:
		return FLinearColor::White;
	}
}