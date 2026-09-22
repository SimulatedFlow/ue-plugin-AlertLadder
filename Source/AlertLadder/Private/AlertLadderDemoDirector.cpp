// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "AlertLadderDemoDirector.h"

#include "AlertLadderStatics.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

namespace AlertLadderDemoLocal
{
	/**
	 * How far in front of the actor the stage starts.
	 *
	 * Much larger than in the board demos, and it has to be: the path runs to Y = 700 and the search
	 * circles to another 1100 on top of that, while the actor sits just in front of the wall so its
	 * text components land on the board. With the board demos' 300 the intruder walked straight
	 * through the back wall and spent half the cycle invisible behind it.
	 */
	constexpr float FloorY = 1400.0f;
	constexpr float Sichtweite = 1100.0f;

	const FColor RungFarbe[4] = {
		FColor(140, 148, 164),   // Unaware
		FColor(230, 208, 110),   // Suspicious
		FColor(235, 150, 80),    // Searching
		FColor(232, 96, 90)      // Alerted
	};
	const FColor Sicht(120, 170, 235);
	const FColor Letzte(240, 225, 130);
	const FColor Rahmen(58, 60, 70);
	const FColor Text(212, 218, 232);

	/**
	 * The intruder's path, as (second, X, Y). Lerped between keys.
	 *
	 * POSITIVE X is the left of the picture: the camera looks along +Y with +Z up, which mirrors the
	 * X axis on screen. The guard called LEFT therefore stands at positive X, and the intruder walks
	 * from positive X to negative - left to right, the way the board reads.
	 */
	struct FSchritt { float T; float X; float Y; };

	static const FSchritt Pfad[] = {
		{ 0.0f,  2200.0f,  200.0f},
		{ 5.0f,   700.0f,  180.0f},   // into the left guard's cone
		{ 9.0f,    80.0f,  140.0f},   // in plain sight
		{12.0f,   600.0f,  620.0f},   // breaks contact behind cover
		{17.0f,  1500.0f,  700.0f},   // stays out of sight while they search
		{22.0f,  -700.0f,  160.0f},   // reappears on the right
		{26.0f, -1500.0f,  200.0f},
		{30.0f, -2300.0f,  260.0f},
	};
	constexpr int32 PfadLaenge = UE_ARRAY_COUNT(Pfad);
}

AAlertLadderDemoDirector::AAlertLadderDemoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(Root);
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextBottom);
	BoardText->SetWorldSize(72.0f);
	BoardText->SetTextRenderColor(FColor::White);
	// Yaw 270, not 90: at 90 a TextRender renders mirrored.
	BoardText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	BoardText->SetRelativeLocation(FVector(0.0f, 0.0f, 1080.0f));

	StateText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StateText"));
	StateText->SetupAttachment(Root);
	StateText->SetHorizontalAlignment(EHTA_Center);
	StateText->SetVerticalAlignment(EVRTA_TextTop);
	StateText->SetWorldSize(48.0f);
	StateText->SetTextRenderColor(AlertLadderDemoLocal::Text);
	StateText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	StateText->SetRelativeLocation(FVector(-1050.0f, 0.0f, 950.0f));

	RuleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RuleText"));
	RuleText->SetupAttachment(Root);
	RuleText->SetHorizontalAlignment(EHTA_Center);
	RuleText->SetVerticalAlignment(EVRTA_TextTop);
	RuleText->SetWorldSize(50.0f);
	RuleText->SetTextRenderColor(FColor(250, 205, 120));
	RuleText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	RuleText->SetRelativeLocation(FVector(1060.0f, 0.0f, 950.0f));
}

FVector AAlertLadderDemoDirector::Basis() const
{
	return GetActorLocation() - FVector(0.0f, AlertLadderDemoLocal::FloorY, GetActorLocation().Z);
}

void AAlertLadderDemoDirector::BeginPlay()
{
	Super::BeginPlay();
	StartCycle();
}

void AAlertLadderDemoDirector::StartCycle()
{
	CycleTime = 0.0f;
	bAlarmLaeuft = false;

	Regeln = FAlertRules();
	Regeln.SuspiciousAt = 0.25f;
	Regeln.SearchingAt = 0.55f;
	Regeln.AlertedAt = 0.85f;
	Regeln.Hysteresis = 0.08f;
	Regeln.RisePerSecond = 0.6f;
	Regeln.CalmDelaySeconds = 3.0f;
	Regeln.FallPerSecond = 0.10f;
	Regeln.InitialSearchRadius = 250.0f;
	Regeln.SearchGrowthPerSecond = 140.0f;
	// Kept short enough that a fully spread search still lies on the floor instead of inside the
	// back wall.
	Regeln.MaxSearchRadius = 1100.0f;
	Regeln = UAlertLadderStatics::NormaliseRules(Regeln);

	Wachen.Reset();

	FWache Links;
	Links.Name = TEXT("LEFT");
	Links.Ort = FVector(500.0f, 0.0f, 0.0f);      // positive X is the left of the picture
	Wachen.Add(Links);

	FWache Rechts;
	Rechts.Name = TEXT("RIGHT");
	Rechts.Ort = FVector(-900.0f, 0.0f, 0.0f);
	Wachen.Add(Rechts);

	bBereit = true;
}

FVector AAlertLadderDemoDirector::ZielOrt(const float T) const
{
	using namespace AlertLadderDemoLocal;

	if (T <= Pfad[0].T)
	{
		return FVector(Pfad[0].X, Pfad[0].Y, 0.0f);
	}
	for (int32 i = 1; i < PfadLaenge; ++i)
	{
		if (T <= Pfad[i].T)
		{
			const float Spanne = FMath::Max(KINDA_SMALL_NUMBER, Pfad[i].T - Pfad[i - 1].T);
			const float A = (T - Pfad[i - 1].T) / Spanne;
			return FVector(FMath::Lerp(Pfad[i - 1].X, Pfad[i].X, A),
				FMath::Lerp(Pfad[i - 1].Y, Pfad[i].Y, A), 0.0f);
		}
	}
	return FVector(Pfad[PfadLaenge - 1].X, Pfad[PfadLaenge - 1].Y, 0.0f);
}

bool AAlertLadderDemoDirector::SichtVon(const FWache& W, const FVector& Zielort) const
{
	// Range, and "in front of the wall" - the intruder's Y above 400 counts as behind cover. A
	// deliberately crude stand-in: what counts as seen is the project's business, and the plugin
	// is told the answer rather than working it out.
	if (Zielort.Y > 400.0f)
	{
		return false;
	}
	return FVector::Dist2D(W.Ort, Zielort) <= AlertLadderDemoLocal::Sichtweite;
}

void AAlertLadderDemoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAutoRun)
	{
		StepDemo(FMath::Clamp(DeltaSeconds, 0.0f, 0.1f));
	}
}

void AAlertLadderDemoDirector::StepDemo(const float Seconds)
{
	// BeginPlay does not run in an editor viewport.
	if (!bBereit)
	{
		StartCycle();
	}

	CycleTime += Seconds;
	if (CycleTime > CycleSeconds)
	{
		StartCycle();
		return;
	}

	Ziel = ZielOrt(CycleTime);

	for (FWache& W : Wachen)
	{
		W.bSiehtGerade = SichtVon(W, Ziel);

		if (W.bSiehtGerade)
		{
			// Strength falls off with distance: a figure at the edge of the cone is not the same
			// stimulus as one standing in front of you.
			const float Weg = FVector::Dist2D(W.Ort, Ziel);
			const float Staerke = FMath::Clamp(1.0f - Weg / AlertLadderDemoLocal::Sichtweite, 0.1f, 1.0f);

			FAlertState Neu = UAlertLadderStatics::ApplyStimulus(W.Stand, Staerke, Seconds, Regeln);

			// The last known position moves ONLY while the target is actually visible.
			Neu.LastKnownPosition = Ziel;
			Neu.bHasLastKnown = true;
			W.Stand = Neu;
		}
		else
		{
			W.Stand = UAlertLadderStatics::Advance(W.Stand, Seconds, Regeln);
		}
	}

	// The shared alarm: anybody who is Alerted pulls the rest up to Searching - and never pulls
	// anybody down.
	//
	// RAISED ON THE EDGE, not every step. An alarm re-applied each frame overwrites the other
	// guard's awareness with the floor value the instant it decays one point below the rung, so
	// it is yanked straight back up: the first take had the second guard sawing between
	// forty-seven and fifty-five percent for seven seconds straight. Real code calls this once,
	// when somebody starts shouting.
	EAlertState Hoechste = EAlertState::Unaware;
	FVector Wo = FVector::ZeroVector;
	bool bWo = false;
	for (const FWache& W : Wachen)
	{
		if (static_cast<int32>(W.Stand.Rung) > static_cast<int32>(Hoechste))
		{
			Hoechste = W.Stand.Rung;
			Wo = W.Stand.LastKnownPosition;
			bWo = W.Stand.bHasLastKnown;
		}
	}
	if (Hoechste != EAlertState::Alerted)
	{
		bAlarmLaeuft = false;
	}
	else if (bWo && !bAlarmLaeuft)
	{
		bAlarmLaeuft = true;
		for (FWache& W : Wachen)
		{
			const EAlertState Neu = UAlertLadderStatics::RaiseToFloor(W.Stand.Rung, EAlertState::Searching);
			if (Neu != W.Stand.Rung)
			{
				W.Stand.Rung = Neu;
				W.Stand.Awareness = FMath::Max(W.Stand.Awareness,
					UAlertLadderStatics::AwarenessForRung(Neu, Regeln));
				W.Stand.SecondsSinceStimulus = 0.0f;
				if (!W.Stand.bHasLastKnown)
				{
					W.Stand.LastKnownPosition = Wo;
					W.Stand.bHasLastKnown = true;
				}
			}
		}
	}

	if (BoardText)
	{
		BoardText->SetText(FText::FromString(HeadlineFor()));
	}
	if (StateText)
	{
		StateText->SetText(FText::FromString(BuildStateText()));
	}
	if (RuleText)
	{
		RuleText->SetText(FText::FromString(RuleFor()));
	}

	if (bDrawDemo)
	{
		DrawScene();
	}
}

FString AAlertLadderDemoDirector::HeadlineFor() const
{
	if (CycleTime < 11.0f)
	{
		return TEXT("noticing takes under two seconds - forgetting takes over ten");
	}
	if (CycleTime < 21.0f)
	{
		return TEXT("contact is lost: the search starts where the intruder WAS, and spreads");
	}
	return TEXT("the ladder is climbed in one step and descended one rung at a time");
}

FString AAlertLadderDemoDirector::RuleFor() const
{
	if (CycleTime < 11.0f)
	{
		return TEXT("THE ASYMMETRY\n\nan AI that forgets as fast\nas it notices can be\nout-waited behind a crate\n\nso rising is six times\nfaster than falling");
	}
	if (CycleTime < 21.0f)
	{
		return TEXT("WHERE IT WAS\n\nthe marker stops moving\nthe moment sight is lost\n\nupdating it afterwards is\nthe difference between\nsearching and cheating");
	}
	return TEXT("ONE RUNG AT A TIME\n\nalerted, searching,\nsuspicious, unaware\n\na guard that forgets\neverything in one frame is\nthe oldest bug in stealth");
}

FString AAlertLadderDemoDirector::BuildStateText() const
{
	FString S = TEXT("THE GUARDS\n");
	for (const FWache& W : Wachen)
	{
		S += FString::Printf(TEXT("%-6s %-11s %3.0f%%  %s\n"),
			*W.Name, *UEnum::GetValueAsString(W.Stand.Rung).Replace(TEXT("EAlertState::"), TEXT("")),
			W.Stand.Awareness * 100.0f,
			W.bSiehtGerade ? TEXT("SEES YOU") : TEXT(""));
	}

	S += TEXT("\nSINCE ANY STIMULUS\n");
	for (const FWache& W : Wachen)
	{
		S += FString::Printf(TEXT("%-6s %5.1fs %s\n"), *W.Name, W.Stand.SecondsSinceStimulus,
			W.Stand.SecondsSinceStimulus < Regeln.CalmDelaySeconds
				? TEXT("(not yet forgetting)") : TEXT(""));
	}

	S += TEXT("\nSEARCH\n");
	for (const FWache& W : Wachen)
	{
		S += FString::Printf(TEXT("%-6s %5.1fs  r = %4.0f\n"), *W.Name, W.Stand.SecondsSearching,
			UAlertLadderStatics::SearchRadius(W.Stand.SecondsSearching, Regeln));
	}
	return S;
}

void AAlertLadderDemoDirector::DrawScene() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UWorld* Mutable = const_cast<UWorld*>(World);

	// Persistent lines plus a flush per step - a lifetime is counted down by the world tick, and a
	// viewport that is not set to realtime never ticks.
	FlushPersistentDebugLines(Mutable);

	using namespace AlertLadderDemoLocal;

	const FVector B = Basis();
	const FVector AchseX(1.0f, 0.0f, 0.0f);
	const FVector AchseY(0.0f, 1.0f, 0.0f);

	for (const FWache& W : Wachen)
	{
		const int32 Stufe = FMath::Clamp(static_cast<int32>(W.Stand.Rung), 0, 3);
		const FColor Farbe = RungFarbe[Stufe];
		const FVector Posten = B + W.Ort + FVector(0.0f, 0.0f, 60.0f);

		DrawDebugSphere(Mutable, Posten, 80.0f, 12, Farbe, true, -1.0f, 0, 7.0f);

		// The awareness bar over the guard's head.
		constexpr float BalkenHoehe = 320.0f;
		for (int32 Ply = 0; Ply < 4; ++Ply)
		{
			const FVector Unten = Posten + FVector((Ply - 1.5f) * 15.0f, 0.0f, 120.0f);
			DrawDebugLine(Mutable, Unten, Unten + FVector(0.0f, 0.0f, BalkenHoehe),
				Rahmen, true, -1.0f, 0, 4.0f);
			if (W.Stand.Awareness > 0.0f)
			{
				DrawDebugLine(Mutable, Unten,
					Unten + FVector(0.0f, 0.0f, BalkenHoehe * W.Stand.Awareness),
					Farbe, true, -1.0f, 0, 11.0f);
			}
		}

		// The three thresholds on that bar, so the rung and the number agree on screen.
		const float Schwelle[3] = {Regeln.SuspiciousAt, Regeln.SearchingAt, Regeln.AlertedAt};
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector M = Posten + FVector(-26.0f, 0.0f, 120.0f + BalkenHoehe * Schwelle[i]);
			DrawDebugLine(Mutable, M, M + FVector(52.0f, 0.0f, 0.0f),
				RungFarbe[i + 1], true, -1.0f, 0, 3.0f);
		}

		// A line to the intruder while it is actually visible.
		if (W.bSiehtGerade)
		{
			DrawDebugLine(Mutable, Posten, B + Ziel + FVector(0.0f, 0.0f, 300.0f),
				Sicht, true, -1.0f, 0, 5.0f);
		}

		// The last known position, and the circle the search has spread to.
		if (W.Stand.bHasLastKnown && W.Stand.Rung != EAlertState::Unaware)
		{
			// A post with a cross at its foot: the guard's belief about where the intruder is has to
			// be as readable on the floor as the intruder itself, or the gap between the two - the
			// entire point of the plugin - is invisible.
			const FVector Wo = B + W.Stand.LastKnownPosition + FVector(0.0f, 0.0f, 6.0f);
			DrawDebugLine(Mutable, Wo, Wo + FVector(0.0f, 0.0f, 220.0f), Letzte, true, -1.0f, 0, 7.0f);
			DrawDebugLine(Mutable, Wo - FVector(110.0f, 0.0f, 0.0f), Wo + FVector(110.0f, 0.0f, 0.0f),
				Letzte, true, -1.0f, 0, 6.0f);
			DrawDebugLine(Mutable, Wo - FVector(0.0f, 110.0f, 0.0f), Wo + FVector(0.0f, 110.0f, 0.0f),
				Letzte, true, -1.0f, 0, 6.0f);

			if (W.Stand.Rung == EAlertState::Searching)
			{
				DrawDebugCircle(Mutable, Wo,
					UAlertLadderStatics::SearchRadius(W.Stand.SecondsSearching, Regeln), 48,
					RungFarbe[2], true, -1.0f, 0, 5.0f, AchseX, AchseY, false);
			}
		}
	}

	// The intruder, on a stem: the ball rides above head height so it stays visible over the cover.
	// Hiding the intruder completely would hide the very thing the guards are failing to see.
	const FVector Z = B + Ziel + FVector(0.0f, 0.0f, 300.0f);
	DrawDebugSphere(Mutable, Z, 85.0f, 14, Letzte, true, -1.0f, 0, 9.0f);
	DrawDebugLine(Mutable, B + Ziel, Z, Letzte, true, -1.0f, 0, 5.0f);
#endif
}
