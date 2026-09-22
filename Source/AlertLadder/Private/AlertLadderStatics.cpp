// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "AlertLadderStatics.h"

namespace AlertLadderLocal
{
	/** The three entry thresholds in rung order, so the walk below can be a loop. */
	static void Schwellen(const FAlertRules& R, float Out[3])
	{
		Out[0] = R.SuspiciousAt;
		Out[1] = R.SearchingAt;
		Out[2] = R.AlertedAt;
	}
}

FAlertRules UAlertLadderStatics::NormaliseRules(const FAlertRules& Rules)
{
	FAlertRules Out = Rules;

	Out.SuspiciousAt = FMath::Clamp(Out.SuspiciousAt, 0.0f, 1.0f);
	Out.SearchingAt = FMath::Clamp(Out.SearchingAt, 0.0f, 1.0f);
	Out.AlertedAt = FMath::Clamp(Out.AlertedAt, 0.0f, 1.0f);

	// Strictly ascending. Thresholds out of order would make a rung unreachable, and an unreachable
	// rung is a bark nobody ever hears and a search nobody ever starts.
	Out.SearchingAt = FMath::Max(Out.SearchingAt, Out.SuspiciousAt);
	Out.AlertedAt = FMath::Max(Out.AlertedAt, Out.SearchingAt);

	// The band has to fit between two thresholds, or leaving a rung would drop straight past the
	// one below it and the "never skip downwards" rule would be doing all the work alone.
	const float Engste = FMath::Min(Out.SearchingAt - Out.SuspiciousAt,
		Out.AlertedAt - Out.SearchingAt);
	const float Grenze = FMath::Max(0.0f, FMath::Min(Engste, Out.SuspiciousAt));
	Out.Hysteresis = FMath::Clamp(Out.Hysteresis, 0.0f, FMath::Max(0.0f, Grenze));

	Out.RisePerSecond = FMath::Max(0.0f, Out.RisePerSecond);
	Out.FallPerSecond = FMath::Max(0.0f, Out.FallPerSecond);
	Out.CalmDelaySeconds = FMath::Max(0.0f, Out.CalmDelaySeconds);
	Out.InitialSearchRadius = FMath::Max(0.0f, Out.InitialSearchRadius);
	Out.MaxSearchRadius = FMath::Max(Out.MaxSearchRadius, Out.InitialSearchRadius);
	Out.SearchGrowthPerSecond = FMath::Max(0.0f, Out.SearchGrowthPerSecond);

	return Out;
}

EAlertState UAlertLadderStatics::NextRung(const float Awareness, const EAlertState Current,
	const FAlertRules& Rules)
{
	const FAlertRules R = NormaliseRules(Rules);
	float Schwelle[3];
	AlertLadderLocal::Schwellen(R, Schwelle);

	const int32 Jetzt = static_cast<int32>(Current);

	// Climbing: as many rungs as the awareness has actually earned. A single enormous stimulus
	// should be able to take a guard from unaware to alerted, because that is what being shot is.
	int32 Erreicht = 0;
	for (int32 i = 0; i < 3; ++i)
	{
		if (Awareness >= Schwelle[i])
		{
			Erreicht = i + 1;
		}
	}
	if (Erreicht > Jetzt)
	{
		return static_cast<EAlertState>(Erreicht);
	}

	// Descending: ONE rung at a time, and only once the awareness is a whole band below the
	// threshold that was crossed to get here.
	if (Jetzt > 0 && Awareness < Schwelle[Jetzt - 1] - R.Hysteresis)
	{
		return static_cast<EAlertState>(Jetzt - 1);
	}

	return Current;
}

float UAlertLadderStatics::Rise(const float Awareness, const float StimulusStrength,
	const float DeltaSeconds, const FAlertRules& Rules)
{
	const FAlertRules R = NormaliseRules(Rules);
	const float Staerke = FMath::Clamp(StimulusStrength, 0.0f, 1.0f);
	const float Zu = R.RisePerSecond * Staerke * FMath::Max(0.0f, DeltaSeconds);
	return FMath::Clamp(Awareness + Zu, 0.0f, 1.0f);
}

float UAlertLadderStatics::Fall(const float Awareness, const float SecondsSinceStimulusAtStart,
	const float DeltaSeconds, const FAlertRules& Rules)
{
	const FAlertRules R = NormaliseRules(Rules);
	const float Schritt = FMath::Max(0.0f, DeltaSeconds);
	const float Vorher = FMath::Max(0.0f, SecondsSinceStimulusAtStart);
	const float Nachher = Vorher + Schritt;

	// Only the part of the step that lies PAST the delay counts.
	//
	// WARUM (14.09.2026, von einem roten Test gefunden): vorher stand hier eine Abfrage auf den
	// Stand am ENDE des Schritts, und dann verfiel die ganze Schrittlaenge. Ein Schritt von 2,0 s
	// ueber eine Verzoegerung von 4,0 s hinweg vergass damit doppelt so viel wie zwei Schritte von
	// 1,0 s - die Bildrate haette also entschieden, wie schnell eine Wache sich beruhigt.
	const float Wirksam = FMath::Clamp(Nachher - FMath::Max(Vorher, R.CalmDelaySeconds), 0.0f, Schritt);
	if (Wirksam <= 0.0f)
	{
		return FMath::Clamp(Awareness, 0.0f, 1.0f);
	}

	return FMath::Clamp(Awareness - R.FallPerSecond * Wirksam, 0.0f, 1.0f);
}

float UAlertLadderStatics::SearchRadius(const float SecondsSearching, const FAlertRules& Rules)
{
	const FAlertRules R = NormaliseRules(Rules);
	const float Gewachsen = R.InitialSearchRadius
		+ R.SearchGrowthPerSecond * FMath::Max(0.0f, SecondsSearching);
	return FMath::Min(Gewachsen, R.MaxSearchRadius);
}

EAlertState UAlertLadderStatics::RaiseToFloor(const EAlertState Current, const EAlertState Floor)
{
	return static_cast<int32>(Floor) > static_cast<int32>(Current) ? Floor : Current;
}

float UAlertLadderStatics::AwarenessForRung(const EAlertState Rung, const FAlertRules& Rules)
{
	const FAlertRules R = NormaliseRules(Rules);
	switch (Rung)
	{
	case EAlertState::Suspicious: return R.SuspiciousAt;
	case EAlertState::Searching:  return R.SearchingAt;
	case EAlertState::Alerted:    return R.AlertedAt;
	default:                      return 0.0f;
	}
}

FAlertState UAlertLadderStatics::Advance(const FAlertState& State, const float DeltaSeconds,
	const FAlertRules& Rules)
{
	FAlertState Out = State;
	if (DeltaSeconds <= 0.0f)
	{
		return Out;
	}

	// Fall gets the value at the START of the step - see the comment in Fall for why that matters.
	Out.Awareness = Fall(State.Awareness, State.SecondsSinceStimulus, DeltaSeconds, Rules);
	Out.SecondsSinceStimulus = State.SecondsSinceStimulus + DeltaSeconds;

	const EAlertState Neu = NextRung(Out.Awareness, State.Rung, Rules);
	Out.Rung = Neu;

	// The search clock runs only while searching, and starts from zero each time that rung is
	// entered - a guard who searched a minute ago does not resume with a mile-wide radius.
	if (Neu == EAlertState::Searching)
	{
		Out.SecondsSearching = (State.Rung == EAlertState::Searching)
			? State.SecondsSearching + DeltaSeconds : 0.0f;
	}
	else
	{
		Out.SecondsSearching = 0.0f;
	}

	return Out;
}

FAlertState UAlertLadderStatics::ApplyStimulus(const FAlertState& State,
	const float StimulusStrength, const float DeltaSeconds, const FAlertRules& Rules)
{
	FAlertState Out = State;

	Out.Awareness = Rise(State.Awareness, StimulusStrength, DeltaSeconds, Rules);
	Out.SecondsSinceStimulus = 0.0f;

	const EAlertState Neu = NextRung(Out.Awareness, State.Rung, Rules);
	if (Neu == EAlertState::Alerted && State.Rung != EAlertState::Alerted)
	{
		Out.TimesAlerted = State.TimesAlerted + 1;
	}
	Out.Rung = Neu;

	if (Neu == EAlertState::Searching)
	{
		Out.SecondsSearching = (State.Rung == EAlertState::Searching)
			? State.SecondsSearching + FMath::Max(0.0f, DeltaSeconds) : 0.0f;
	}
	else
	{
		Out.SecondsSearching = 0.0f;
	}

	return Out;
}
