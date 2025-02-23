// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameSettingValueDiscrete.h"

#include "ArenaSettingValueDiscrete_PerfStat.generated.h"

enum class EArenaDisplayablePerformanceStat : uint8;
enum class EArenaStatDisplayMode : uint8;

class UObject;

UCLASS()
class UArenaSettingValueDiscrete_PerfStat : public UGameSettingValueDiscrete
{
	GENERATED_BODY()
	
public:

	UArenaSettingValueDiscrete_PerfStat();

	void SetStat(EArenaDisplayablePerformanceStat InStat);

	/** UGameSettingValue */
	virtual void StoreInitial() override;
	virtual void ResetToDefault() override;
	virtual void RestoreToInitial() override;

	/** UGameSettingValueDiscrete */
	virtual void SetDiscreteOptionByIndex(int32 Index) override;
	virtual int32 GetDiscreteOptionIndex() const override;
	virtual TArray<FText> GetDiscreteOptions() const override;

protected:
	/** UGameSettingValue */
	virtual void OnInitialized() override;
	
	void AddMode(FText&& Label, EArenaStatDisplayMode Mode);
protected:
	TArray<FText> Options;
	TArray<EArenaStatDisplayMode> DisplayModes;

	EArenaDisplayablePerformanceStat StatToDisplay;
	EArenaStatDisplayMode InitialMode;
};
