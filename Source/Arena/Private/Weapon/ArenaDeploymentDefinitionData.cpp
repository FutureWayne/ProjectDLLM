// Copyright Ludens Studio. All Rights Reserved.


#include "Weapon/ArenaDeploymentDefinitionData.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UArenaDeploymentDefinitionData::IsDataValid(class FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (DeployActor == nullptr)
	{
		Context.AddError(NSLOCTEXT("Arena", "DeployActorClassNotSet", "Deploy Actor is not set."));
		Result = EDataValidationResult::Invalid;
	}
	
	return Result;
}
#endif


