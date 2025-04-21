// Copyright Ludens Studio. All Rights Reserved.


#include "Development/ArenaDeveloperSettings.h"

UArenaDeveloperSettings::UArenaDeveloperSettings()
{
}

FName UArenaDeveloperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}
