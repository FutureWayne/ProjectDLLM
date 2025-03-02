// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/IndicatorSystem/IndicatorLibrary.h"

#include "UI/IndicatorSystem/ArenaIndicatorManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IndicatorLibrary)

class AController;

UIndicatorLibrary::UIndicatorLibrary()
{
}

UArenaIndicatorManagerComponent* UIndicatorLibrary::GetIndicatorManagerComponent(AController* Controller)
{
	return UArenaIndicatorManagerComponent::GetComponent(Controller);
}

