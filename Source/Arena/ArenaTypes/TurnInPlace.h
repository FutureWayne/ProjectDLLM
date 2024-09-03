#pragma once

UENUM(BlueprintType)
enum class ETurnInPlaceDirection : uint8
{
	ETIP_Left UMETA(DisplayName = "Left"),
	ETIP_Right UMETA(DisplayName = "Right"),
	ETIP_NotTurning UMETA(DisplayName = "NotTurning"),

	ETIP_MAX UMETA(DisplayName = "DefaultMax")
};
