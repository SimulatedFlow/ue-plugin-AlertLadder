// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlertLadderTypes.h"
#include "AlertLadderStatics.generated.h"

/**
 * The rules, on their own.
 *
 * No world, no actor, no perception, no clock. The component calls exactly these and so do the
 * tests, which is the only way the debug draw and the behaviour cannot drift apart.
 */
UCLASS()
class ALERTLADDER_API UAlertLadderStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Rules with the thresholds sorted and the hysteresis kept small enough to fit between them. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static FAlertRules NormaliseRules(const FAlertRules& Rules);

	/**
	 * Which rung this awareness means, given the rung it is on now.
	 *
	 * The current rung is an input because the thresholds are asymmetric: climbing needs the full
	 * threshold, descending needs to fall a whole hysteresis band below it. A guard standing on a
	 * boundary would otherwise change state every frame, and every change fires a bark.
	 *
	 * It also never skips downwards. From Alerted you pass through Searching and Suspicious, one
	 * rung per call, because an AI that goes from hunting to humming in one frame is the oldest
	 * bug in stealth.
	 */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static EAlertState NextRung(float Awareness, EAlertState Current, const FAlertRules& Rules);

	/** Awareness after a stimulus of the given strength for DeltaSeconds. Clamped to one. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static float Rise(float Awareness, float StimulusStrength, float DeltaSeconds,
		const FAlertRules& Rules);

	/**
	 * Awareness after DeltaSeconds of nothing happening.
	 *
	 * Falls only once the delay has passed, and far more slowly than it rose.
	 *
	 * SecondsSinceStimulusAtStart is the value at the START of the step, not the end. Only the part
	 * of the step that lies past the delay counts, so a step that straddles the boundary decays for
	 * the right fraction of itself. Passing the end value instead makes one long step forget more
	 * than several short ones covering the same time - which means the frame rate would change how
	 * quickly a guard calms down.
	 */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static float Fall(float Awareness, float SecondsSinceStimulusAtStart, float DeltaSeconds,
		const FAlertRules& Rules);

	/** How wide the search has spread after this long looking. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static float SearchRadius(float SecondsSearching, const FAlertRules& Rules);

	/**
	 * Raise a rung to at least the floor - never lower it.
	 *
	 * This is how a shared alarm works. Setting the group to the caller's level instead would calm
	 * down the one guard who is already fighting, which is precisely backwards.
	 */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static EAlertState RaiseToFloor(EAlertState Current, EAlertState Floor);

	/** The awareness that corresponds to a rung's entry threshold. For a shared alarm's number. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static float AwarenessForRung(EAlertState Rung, const FAlertRules& Rules);

	/** Time passing, with no stimulus. Awareness falls, the rung follows, the timers run. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static FAlertState Advance(const FAlertState& State, float DeltaSeconds, const FAlertRules& Rules);

	/** One stimulus this frame: awareness rises, the delay resets, the rung follows. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder|Rules")
	static FAlertState ApplyStimulus(const FAlertState& State, float StimulusStrength,
		float DeltaSeconds, const FAlertRules& Rules);
};
