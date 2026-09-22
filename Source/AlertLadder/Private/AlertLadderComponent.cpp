// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "AlertLadderComponent.h"

#include "AlertLadderLog.h"
#include "AlertLadderSettings.h"
#include "AlertLadderStatics.h"
#include "GameFramework/Actor.h"

UAlertLadderComponent::UAlertLadderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

FAlertRules UAlertLadderComponent::GetRules() const
{
	return UAlertLadderStatics::NormaliseRules(
		bOverrideRules ? RuleOverride : UAlertLadderSettings::Get()->Rules);
}

float UAlertLadderComponent::GetSearchRadius() const
{
	return UAlertLadderStatics::SearchRadius(State.SecondsSearching, GetRules());
}

void UAlertLadderComponent::Uebernehmen(const FAlertState& Neu, const EAlertChange Warum)
{
	const EAlertState Vorher = State.Rung;
	const bool bWarSuchend = State.Rung == EAlertState::Searching;

	State = Neu;

	if (State.Rung == Vorher)
	{
		return;
	}

	if (UAlertLadderSettings::Get()->bLogChanges)
	{
		UE_LOG(LogAlertLadder, Display, TEXT("[%s] %s -> %s (%s), awareness %.2f"),
			*GetNameSafe(GetOwner()), *UEnum::GetValueAsString(Vorher),
			*UEnum::GetValueAsString(State.Rung), *UEnum::GetValueAsString(Warum), State.Awareness);
	}

	OnRungChanged.Broadcast(Vorher, State.Rung, Warum);

	if (State.Rung == EAlertState::Searching && !bWarSuchend && State.bHasLastKnown)
	{
		OnSearchStarted.Broadcast(State.LastKnownPosition);
	}
}

void UAlertLadderComponent::ReportStimulus(const float Strength, const float DeltaSeconds)
{
	Uebernehmen(UAlertLadderStatics::ApplyStimulus(State, Strength, DeltaSeconds, GetRules()),
		EAlertChange::Escalated);
}

void UAlertLadderComponent::ReportSeen(const FVector TargetLocation, const float Strength,
	const float DeltaSeconds)
{
	FAlertState Neu = UAlertLadderStatics::ApplyStimulus(State, Strength, DeltaSeconds, GetRules());

	// The last known position moves ONLY here, while the target is actually visible. Moving it
	// afterwards is the difference between a guard searching and a guard cheating, and a player
	// can feel that difference immediately.
	Neu.LastKnownPosition = TargetLocation;
	Neu.bHasLastKnown = true;

	Uebernehmen(Neu, EAlertChange::Escalated);
}

void UAlertLadderComponent::ShareAlarm(const EAlertState Floor, const FVector LastKnown,
	const bool bTakeTheirPosition)
{
	FAlertState Neu = State;

	// Raise, never lower. Setting the rung outright would calm down the one guard who is already
	// fighting - which is precisely backwards.
	Neu.Rung = UAlertLadderStatics::RaiseToFloor(State.Rung, Floor);

	// The awareness follows the rung the same way, so the two cannot end up disagreeing.
	Neu.Awareness = FMath::Max(State.Awareness,
		UAlertLadderStatics::AwarenessForRung(Neu.Rung, GetRules()));
	Neu.SecondsSinceStimulus = 0.0f;

	if (bTakeTheirPosition && !State.bHasLastKnown)
	{
		Neu.LastKnownPosition = LastKnown;
		Neu.bHasLastKnown = true;
	}

	Uebernehmen(Neu, EAlertChange::Shared);
}

void UAlertLadderComponent::RaiseGroup(const EAlertState Floor)
{
	for (const TWeakObjectPtr<UAlertLadderComponent>& Mitglied : Group)
	{
		if (UAlertLadderComponent* M = Mitglied.Get())
		{
			M->ShareAlarm(Floor, State.LastKnownPosition, State.bHasLastKnown);
		}
	}
}

void UAlertLadderComponent::JoinGroup(UAlertLadderComponent* Other)
{
	if (!Other || Other == this)
	{
		return;
	}
	Group.AddUnique(Other);
	Other->Group.AddUnique(this);
}

void UAlertLadderComponent::LeaveGroup()
{
	for (const TWeakObjectPtr<UAlertLadderComponent>& Mitglied : Group)
	{
		if (UAlertLadderComponent* M = Mitglied.Get())
		{
			M->Group.Remove(this);
		}
	}
	Group.Reset();
}

void UAlertLadderComponent::ResetAlert()
{
	State = FAlertState();
}

void UAlertLadderComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoTick)
	{
		AdvanceTime(DeltaTime);
	}
}

void UAlertLadderComponent::AdvanceTime(const float DeltaSeconds)
{
	Uebernehmen(UAlertLadderStatics::Advance(State, DeltaSeconds, GetRules()), EAlertChange::Calmed);
}
