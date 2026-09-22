// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "AlertLadderSettings.h"

UAlertLadderSettings::UAlertLadderSettings()
	: bLogChanges(false)
{
	// A quarter to look up, half to start searching, most of the way to be certain. Between them
	// enough room that a guard on a boundary is not switching rungs every frame.
	Rules.SuspiciousAt = 0.25f;
	Rules.SearchingAt = 0.55f;
	Rules.AlertedAt = 0.85f;
	Rules.Hysteresis = 0.08f;

	// Noticing takes under two seconds at full stimulus; forgetting takes over ten. The asymmetry
	// is the point: an AI that forgets as fast as it notices can be out-waited behind a crate.
	Rules.RisePerSecond = 0.6f;
	Rules.CalmDelaySeconds = 4.0f;
	Rules.FallPerSecond = 0.08f;

	Rules.InitialSearchRadius = 250.0f;
	Rules.SearchGrowthPerSecond = 120.0f;
	Rules.MaxSearchRadius = 1600.0f;
}

const UAlertLadderSettings* UAlertLadderSettings::Get()
{
	const UAlertLadderSettings* Settings = GetDefault<UAlertLadderSettings>();
	check(Settings);
	return Settings;
}
