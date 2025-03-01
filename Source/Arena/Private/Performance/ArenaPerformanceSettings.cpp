// Copyright Ludens Studio. All Rights Reserved.

#include "Performance/ArenaPerformanceSettings.h"

#include "Engine/PlatformSettingsManager.h"
#include "Misc/EnumRange.h"
#include "Performance/ArenaPerformanceStatTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaPerformanceSettings)

//////////////////////////////////////////////////////////////////////

UArenaPlatformSpecificRenderingSettings::UArenaPlatformSpecificRenderingSettings()
{
	MobileFrameRateLimits.Append({ 20, 30, 45, 60, 90, 120 });
}

const UArenaPlatformSpecificRenderingSettings* UArenaPlatformSpecificRenderingSettings::Get()
{
	UArenaPlatformSpecificRenderingSettings* Result = UPlatformSettingsManager::Get().GetSettingsForPlatform<ThisClass>();
	check(Result);
	return Result;
}

//////////////////////////////////////////////////////////////////////

UArenaPerformanceSettings::UArenaPerformanceSettings()
{
	PerPlatformSettings.Initialize(UArenaPlatformSpecificRenderingSettings::StaticClass());

	CategoryName = TEXT("Game");

	DesktopFrameRateLimits.Append({ 30, 60, 120, 144, 160, 165, 180, 200, 240, 360 });

	// Default to all stats are allowed
	FArenaPerformanceStatGroup& StatGroup = UserFacingPerformanceStats.AddDefaulted_GetRef();
	for (EArenaDisplayablePerformanceStat PerfStat : TEnumRange<EArenaDisplayablePerformanceStat>())
	{
		StatGroup.AllowedStats.Add(PerfStat);
	}
}



