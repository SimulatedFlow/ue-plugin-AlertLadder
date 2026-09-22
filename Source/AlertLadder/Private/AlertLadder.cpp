// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "AlertLadder.h"

#include "AlertLadderComponent.h"
#include "AlertLadderLog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogAlertLadder);

#define LOCTEXT_NAMESPACE "FAlertLadderModule"

namespace
{
	UWorld* AlertLadderConsoleWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void AlertLadderDumpCommand()
	{
		UWorld* World = AlertLadderConsoleWorld();
		if (!World)
		{
			UE_LOG(LogAlertLadder, Warning, TEXT("AlertLadder.Dump: no running world."));
			return;
		}

		int32 Gefunden = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const UAlertLadderComponent* A = It->FindComponentByClass<UAlertLadderComponent>();
			if (!A)
			{
				continue;
			}
			++Gefunden;
			const FAlertState& S = A->GetState();
			UE_LOG(LogAlertLadder, Display,
				TEXT("%-24s %-11s awareness %.2f  quiet %5.1fs  searching %5.1fs r=%6.0f  alerted %dx  %s"),
				*It->GetName(), *UEnum::GetValueAsString(S.Rung), S.Awareness,
				S.SecondsSinceStimulus, S.SecondsSearching, A->GetSearchRadius(), S.TimesAlerted,
				S.bHasLastKnown ? *S.LastKnownPosition.ToCompactString() : TEXT("(no contact yet)"));
		}

		if (Gefunden == 0)
		{
			UE_LOG(LogAlertLadder, Display, TEXT("AlertLadder.Dump: nothing in this level has an alert component."));
		}
	}

	void AlertLadderCalmCommand()
	{
		UWorld* World = AlertLadderConsoleWorld();
		if (!World)
		{
			UE_LOG(LogAlertLadder, Warning, TEXT("AlertLadder.Calm: no running world."));
			return;
		}

		int32 Anzahl = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UAlertLadderComponent* A = It->FindComponentByClass<UAlertLadderComponent>())
			{
				A->ResetAlert();
				++Anzahl;
			}
		}
		UE_LOG(LogAlertLadder, Display, TEXT("AlertLadder.Calm: calmed %d."), Anzahl);
	}

	FAutoConsoleCommand GAlertLadderDump(
		TEXT("AlertLadder.Dump"),
		TEXT("Every AI with an alert component: rung, awareness, timers, search radius, last known position."),
		FConsoleCommandDelegate::CreateStatic(&AlertLadderDumpCommand));

	FAutoConsoleCommand GAlertLadderCalm(
		TEXT("AlertLadder.Calm"),
		TEXT("Put every AI back to unaware. The way out when a level will not settle down."),
		FConsoleCommandDelegate::CreateStatic(&AlertLadderCalmCommand));
}

void FAlertLadderModule::StartupModule()
{
}

void FAlertLadderModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAlertLadderModule, AlertLadder)
