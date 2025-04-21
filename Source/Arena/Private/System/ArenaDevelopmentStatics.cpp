// Copyright Ludens Studio. All Rights Reserved.

#include "System/ArenaDevelopmentStatics.h"

#include "Development/ArenaDeveloperSettings.h"

bool UArenaDevelopmentStatics::ShouldSkipDirectlyToGameplay()
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		return !GetDefault<UArenaDeveloperSettings>()->bTestFullGameFlowInPIE;
	}
#endif
	return false;
}
