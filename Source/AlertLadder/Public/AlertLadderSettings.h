// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AlertLadderTypes.h"
#include "AlertLadderSettings.generated.h"

/** Project Settings > Plugins > AlertLadder. */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "AlertLadder"))
class ALERTLADDER_API UAlertLadderSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAlertLadderSettings();

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UAlertLadderSettings* Get();

	UPROPERTY(config, EditAnywhere, Category = "Rules")
	FAlertRules Rules;

	/** Write a line whenever an AI changes rung. */
	UPROPERTY(config, EditAnywhere, Category = "Diagnostics")
	bool bLogChanges;
};
