// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AlertLadderTypes.generated.h"

/**
 * The ladder. The order matters: it is climbed and descended one rung at a time, and the numeric
 * value is used for "raise the floor, never lower it".
 */
UENUM(BlueprintType)
enum class EAlertState : uint8
{
	/** Nothing has happened. */
	Unaware = 0,
	/** Something did. Look over there, stop patrolling. */
	Suspicious = 1,
	/** Contact was lost and there is a place to look. */
	Searching = 2,
	/** The target is known and the fight is on. */
	Alerted = 3,
};

/** Why the state changed, so a bark or an animation can differ. */
UENUM(BlueprintType)
enum class EAlertChange : uint8
{
	None,
	/** Awareness crossed a threshold upwards. */
	Escalated,
	/** Awareness decayed past a threshold. */
	Calmed,
	/** Somebody else's alarm raised this one's floor. */
	Shared,
};

/** The thresholds and timings. Passed to the pure functions explicitly. */
USTRUCT(BlueprintType)
struct ALERTLADDER_API FAlertRules
{
	GENERATED_BODY()

	/** Awareness needed to ENTER each state, 0..1. Leaving uses the hysteresis below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SuspiciousAt = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SearchingAt = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AlertedAt = 0.85f;

	/**
	 * How far below a threshold awareness must fall before the state is left again.
	 *
	 * Without it a guard sitting exactly on a boundary flickers between two states every frame,
	 * and the bark that goes with the change fires over and over.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Hysteresis = 0.08f;

	/** Awareness gained per second at full stimulus strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0"))
	float RisePerSecond = 0.6f;

	/** Seconds without a stimulus before awareness starts falling. Every stimulus resets it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0"))
	float CalmDelaySeconds = 4.0f;

	/**
	 * Awareness lost per second once the delay has passed.
	 *
	 * Deliberately far slower than the rise. An AI that forgets as fast as it notices is an AI the
	 * player can out-wait by stepping behind a crate for two seconds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0"))
	float FallPerSecond = 0.08f;

	/** How far the search spreads out from the last known position, per second of searching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0"))
	float SearchGrowthPerSecond = 120.0f;

	/** The search never spreads further than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0"))
	float MaxSearchRadius = 1600.0f;

	/** Where the search starts, the moment contact is lost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder", meta = (ClampMin = "0.0"))
	float InitialSearchRadius = 250.0f;
};

/** One AI's alert state. Plain data: no world, no actor, no clock. */
USTRUCT(BlueprintType)
struct ALERTLADDER_API FAlertState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "AlertLadder", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Awareness = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "AlertLadder")
	EAlertState Rung = EAlertState::Unaware;

	/** Reset by every stimulus. Drives the calm-down delay. */
	UPROPERTY(BlueprintReadOnly, Category = "AlertLadder")
	float SecondsSinceStimulus = 1000.0f;

	/** How long this AI has been searching. Drives the radius. */
	UPROPERTY(BlueprintReadOnly, Category = "AlertLadder")
	float SecondsSearching = 0.0f;

	/**
	 * Where the target WAS when contact was lost.
	 *
	 * Not where it is now. Updating this while the target is out of sight is the difference between
	 * a guard searching and a guard cheating, and players can feel it immediately.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "AlertLadder")
	FVector LastKnownPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "AlertLadder")
	bool bHasLastKnown = false;

	/** How often this AI has reached Alerted. For "he has seen me three times" rules. */
	UPROPERTY(BlueprintReadOnly, Category = "AlertLadder")
	int32 TimesAlerted = 0;
};
