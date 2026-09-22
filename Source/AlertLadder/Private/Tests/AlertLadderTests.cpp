// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "AlertLadderStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace AlertLadderTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::EngineFilter;

	static FAlertRules Rules()
	{
		FAlertRules R;
		R.SuspiciousAt = 0.25f;
		R.SearchingAt = 0.55f;
		R.AlertedAt = 0.85f;
		R.Hysteresis = 0.08f;
		R.RisePerSecond = 0.6f;
		R.CalmDelaySeconds = 4.0f;
		R.FallPerSecond = 0.08f;
		R.InitialSearchRadius = 250.0f;
		R.SearchGrowthPerSecond = 120.0f;
		R.MaxSearchRadius = 1600.0f;
		return R;
	}

	static FAlertState Auf(const float Awareness, const EAlertState Rung,
		const float SeitReiz = 1000.0f)
	{
		FAlertState S;
		S.Awareness = Awareness;
		S.Rung = Rung;
		S.SecondsSinceStimulus = SeitReiz;
		return S;
	}
}

// -------------------------------------------------------------------------------------------------
// The oldest bug in stealth.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlertLadderNeverSkipsOnTheWayDown,
	"AlertLadder.Ladder.NeverSkipsARungOnTheWayDown", AlertLadderTests::TestFlags)

bool FAlertLadderNeverSkipsOnTheWayDown::RunTest(const FString&)
{
	using namespace AlertLadderTests;
	const FAlertRules R = Rules();

	// A guard that was hunting, with the awareness suddenly at nothing - the player teleported
	// away, the level streamed, whatever. It must still walk DOWN the ladder rather than forget
	// everything in one frame.
	const EAlertState EinsRunter = UAlertLadderStatics::NextRung(0.0f, EAlertState::Alerted, R);
	TestTrue(TEXT("Alerted steps down to Searching, not to Unaware"),
		EinsRunter == EAlertState::Searching);

	const EAlertState ZweiRunter = UAlertLadderStatics::NextRung(0.0f, EinsRunter, R);
	TestTrue(TEXT("then to Suspicious"), ZweiRunter == EAlertState::Suspicious);

	const EAlertState DreiRunter = UAlertLadderStatics::NextRung(0.0f, ZweiRunter, R);
	TestTrue(TEXT("then, finally, to Unaware"), DreiRunter == EAlertState::Unaware);

	TestTrue(TEXT("and it stays there"),
		UAlertLadderStatics::NextRung(0.0f, DreiRunter, R) == EAlertState::Unaware);

	// Climbing is different on purpose: being shot should be able to take a guard from unaware
	// straight to alerted, because that is what being shot is.
	TestTrue(TEXT("a single big stimulus climbs all the way"),
		UAlertLadderStatics::NextRung(0.95f, EAlertState::Unaware, R) == EAlertState::Alerted);

	return true;
}

// -------------------------------------------------------------------------------------------------
// The boundary must not flicker.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlertLadderHysteresisStopsTheFlicker,
	"AlertLadder.Ladder.HysteresisStopsTheFlickerOnABoundary", AlertLadderTests::TestFlags)

bool FAlertLadderHysteresisStopsTheFlicker::RunTest(const FString&)
{
	using namespace AlertLadderTests;
	const FAlertRules R = Rules();   // Suspicious at 0.25, band 0.08

	// Exactly on the threshold: entering is allowed.
	TestTrue(TEXT("awareness on the threshold enters the rung"),
		UAlertLadderStatics::NextRung(0.25f, EAlertState::Unaware, R) == EAlertState::Suspicious);

	// A hair under it, having just entered: it STAYS. Without the band it would drop straight back,
	// and the "what was that?" bark would fire every frame the player stood still.
	TestTrue(TEXT("a hair below the threshold does not leave the rung"),
		UAlertLadderStatics::NextRung(0.24f, EAlertState::Suspicious, R) == EAlertState::Suspicious);
	TestTrue(TEXT("nor does most of the band"),
		UAlertLadderStatics::NextRung(0.18f, EAlertState::Suspicious, R) == EAlertState::Suspicious);

	// A whole band below: now it leaves.
	TestTrue(TEXT("a whole band below, it leaves"),
		UAlertLadderStatics::NextRung(0.16f, EAlertState::Suspicious, R) == EAlertState::Unaware);

	// Thresholds out of order are corrected rather than trusted - an unreachable rung is a search
	// that never starts and a bark nobody hears.
	FAlertRules Verdreht = R;
	Verdreht.SearchingAt = 0.10f;   // below Suspicious
	Verdreht.AlertedAt = 0.20f;
	const FAlertRules Sauber = UAlertLadderStatics::NormaliseRules(Verdreht);
	TestTrue(TEXT("thresholds come out ascending"),
		Sauber.SuspiciousAt <= Sauber.SearchingAt && Sauber.SearchingAt <= Sauber.AlertedAt);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Noticing is fast, forgetting is slow.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlertLadderForgettingIsSlowAndDelayed,
	"AlertLadder.Awareness.ForgettingIsSlowAndDelayed", AlertLadderTests::TestFlags)

bool FAlertLadderForgettingIsSlowAndDelayed::RunTest(const FString&)
{
	using namespace AlertLadderTests;
	const FAlertRules R = Rules();   // delay 4 s, fall 0.08/s, rise 0.6/s

	// Inside the delay nothing is forgotten at all. Stepping behind a crate for two seconds must
	// not be a way to reset a guard.
	FAlertState S = Auf(0.9f, EAlertState::Alerted, /*SeitReiz=*/0.0f);
	for (int32 i = 0; i < 3; ++i)
	{
		S = UAlertLadderStatics::Advance(S, 1.0f, R);
	}
	TestNearlyEqual(TEXT("three seconds inside the delay change nothing"), S.Awareness, 0.9f, 0.0001f);
	TestTrue(TEXT("and the guard is still alerted"), S.Rung == EAlertState::Alerted);

	// Past the delay it falls - slowly.
	S = UAlertLadderStatics::Advance(S, 2.0f, R);   // t = 5 s, one second of falling
	TestNearlyEqual(TEXT("one second of falling is eight hundredths"), S.Awareness, 0.82f, 0.0001f);

	// THE ASYMMETRY: a single second of stimulus buys far more than a second of quiet takes away.
	const float Dazu = UAlertLadderStatics::Rise(0.5f, 1.0f, 1.0f, R) - 0.5f;
	const float Davon = 0.5f - UAlertLadderStatics::Fall(0.5f, 1000.0f, 1.0f, R);
	TestTrue(TEXT("noticing is much faster than forgetting"), Dazu > Davon * 5.0f);

	// Any stimulus puts the delay back to zero.
	const FAlertState Wieder = UAlertLadderStatics::ApplyStimulus(S, 0.5f, 0.1f, R);
	TestNearlyEqual(TEXT("a stimulus resets the delay"), Wieder.SecondsSinceStimulus, 0.0f, 0.0001f);

	// Awareness never leaves zero-to-one, however long it is left alone.
	FAlertState Lang = Auf(0.2f, EAlertState::Unaware);
	for (int32 i = 0; i < 200; ++i)
	{
		Lang = UAlertLadderStatics::Advance(Lang, 1.0f, R);
	}
	TestTrue(TEXT("awareness never goes negative"), Lang.Awareness >= 0.0f);

	return true;
}

// -------------------------------------------------------------------------------------------------
// The rule a player can feel.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlertLadderSearchGrowsFromWhereTheTargetWas,
	"AlertLadder.Search.GrowsFromWhereTheTargetWas", AlertLadderTests::TestFlags)

bool FAlertLadderSearchGrowsFromWhereTheTargetWas::RunTest(const FString&)
{
	using namespace AlertLadderTests;
	const FAlertRules R = Rules();   // 250 initial, +120/s, capped at 1600

	TestNearlyEqual(TEXT("the search starts at the initial radius"),
		UAlertLadderStatics::SearchRadius(0.0f, R), 250.0f, 0.001f);
	TestNearlyEqual(TEXT("and spreads out"),
		UAlertLadderStatics::SearchRadius(5.0f, R), 850.0f, 0.001f);
	TestNearlyEqual(TEXT("and stops at the cap"),
		UAlertLadderStatics::SearchRadius(600.0f, R), 1600.0f, 0.001f);

	// The search clock runs only while searching, and restarts each time that rung is entered.
	// A guard who searched a minute ago must not resume with a mile-wide radius.
	FAlertState S = Auf(0.6f, EAlertState::Searching);
	S.SecondsSinceStimulus = 0.0f;
	S = UAlertLadderStatics::Advance(S, 3.0f, R);
	TestNearlyEqual(TEXT("three seconds of searching are counted"), S.SecondsSearching, 3.0f, 0.0001f);

	// Climb back to Alerted: the search clock is cleared.
	const FAlertState Wieder = UAlertLadderStatics::ApplyStimulus(S, 1.0f, 1.0f, R);
	TestTrue(TEXT("the guard is alerted again"), Wieder.Rung == EAlertState::Alerted);
	TestNearlyEqual(TEXT("and the search clock is cleared"), Wieder.SecondsSearching, 0.0f, 0.0001f);
	TestEqual(TEXT("and reaching Alerted is counted"), Wieder.TimesAlerted, 1);

	// A cap below the initial radius is a contradiction; the rules resolve it rather than
	// producing a search that shrinks the longer it goes on.
	FAlertRules Eng = R;
	Eng.MaxSearchRadius = 100.0f;
	const FAlertRules Fest = UAlertLadderStatics::NormaliseRules(Eng);
	TestTrue(TEXT("the cap is never below the starting radius"),
		Fest.MaxSearchRadius >= Fest.InitialSearchRadius);

	return true;
}

// -------------------------------------------------------------------------------------------------
// The shared alarm.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlertLadderSharedAlarmRaisesNeverLowers,
	"AlertLadder.Group.SharedAlarmRaisesTheFloorNeverLowersIt", AlertLadderTests::TestFlags)

bool FAlertLadderSharedAlarmRaisesNeverLowers::RunTest(const FString&)
{
	using namespace AlertLadderTests;

	// One guard shouts. Everybody who was calmer comes up to that level.
	TestTrue(TEXT("an unaware guard is pulled up"),
		UAlertLadderStatics::RaiseToFloor(EAlertState::Unaware, EAlertState::Searching)
			== EAlertState::Searching);

	// THE POINT: the guard who is already fighting does NOT come down to it. Setting the group to
	// the caller's level instead of raising a floor calms down exactly the one who should not be.
	TestTrue(TEXT("an alerted guard is not calmed by somebody else's shout"),
		UAlertLadderStatics::RaiseToFloor(EAlertState::Alerted, EAlertState::Suspicious)
			== EAlertState::Alerted);

	TestTrue(TEXT("the same rung stays the same"),
		UAlertLadderStatics::RaiseToFloor(EAlertState::Searching, EAlertState::Searching)
			== EAlertState::Searching);

	// And the awareness that goes with a rung is readable, so a shared alarm can set a number as
	// well as a rung rather than leaving the two disagreeing.
	const FAlertRules R = Rules();
	TestNearlyEqual(TEXT("Searching corresponds to its own threshold"),
		UAlertLadderStatics::AwarenessForRung(EAlertState::Searching, R), 0.55f, 0.0001f);
	TestNearlyEqual(TEXT("Unaware is nothing"),
		UAlertLadderStatics::AwarenessForRung(EAlertState::Unaware, R), 0.0f, 0.0001f);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Determinism.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlertLadderStepSizeDoesNotChangeTheOutcome,
	"AlertLadder.Awareness.TwoHalfStepsEqualOneWholeStep", AlertLadderTests::TestFlags)

bool FAlertLadderStepSizeDoesNotChangeTheOutcome::RunTest(const FString&)
{
	using namespace AlertLadderTests;
	const FAlertRules R = Rules();

	// A frame rate that changes must not change how quickly a guard notices you.
	FAlertState A = Auf(0.1f, EAlertState::Unaware, 0.0f);
	FAlertState B = A;

	A = UAlertLadderStatics::ApplyStimulus(A, 0.7f, 1.0f, R);

	B = UAlertLadderStatics::ApplyStimulus(B, 0.7f, 0.5f, R);
	B = UAlertLadderStatics::ApplyStimulus(B, 0.7f, 0.5f, R);

	TestNearlyEqual(TEXT("one whole step and two half steps agree"), A.Awareness, B.Awareness, 0.0001f);
	TestTrue(TEXT("and land on the same rung"), A.Rung == B.Rung);

	// AND THE SAME FOR FORGETTING, across the calm-down delay - which is where this used to be
	// wrong. A single long step decayed for its whole length as soon as its END was past the
	// delay, so one step of six seconds forgot more than six steps of one. The frame rate would
	// have decided how quickly a guard gave up on you.
	FAlertState Gross = Auf(0.9f, EAlertState::Alerted, /*SeitReiz=*/0.0f);
	FAlertState Klein = Gross;

	Gross = UAlertLadderStatics::Advance(Gross, 6.0f, R);
	for (int32 i = 0; i < 6; ++i)
	{
		Klein = UAlertLadderStatics::Advance(Klein, 1.0f, R);
	}

	TestNearlyEqual(TEXT("one long step and six short ones agree across the delay"),
		Gross.Awareness, Klein.Awareness, 0.0001f);
	TestNearlyEqual(TEXT("and both forgot only the two seconds that lay past it"),
		Gross.Awareness, 0.74f, 0.0001f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
